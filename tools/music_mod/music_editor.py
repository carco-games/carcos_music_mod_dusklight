from wav_editor_lite.track_sync import sync
from wav_editor_lite.arrange_clips import arrange_clips
from wav_editor_lite.json_processor import process_json_folder
from wav_editor_lite.add_silence import add_silence_blocks
from wav_editor_lite.add_amplify import add_amplify_blocks
from pathlib import Path
from pydub import AudioSegment
import imageio_ffmpeg as ffmpeg
import subprocess
import os
import argparse
from io import BytesIO
from tkinter.filedialog import askopenfilename
from gclib.gcm import GCM
from stream_table_editor import BAA
from typing import List

json_folder = process_json_folder(Path("json_data"))
wav_folder = Path("user_provided_wav_files")
wav_files = []
for wav_file in wav_folder.glob("*.wav"):
    wav_files.append(wav_file)
current_wavs_folder = Path("output_wavs")
current_wav_files_stems = []
for wav_file in current_wavs_folder.glob("*.wav"):
    current_wav_files_stems.append(wav_file.stem)

ext_wav_folder = Path("ext_wavs")

# Create output wavs folder if it doesn't exist
path = Path("output_wavs")
path.mkdir(parents=True, exist_ok=True)

# Load iso
BASE_DIR = ((Path(__file__).resolve().parent).resolve().parent).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument("modified_iso_path", nargs="?", default=f"{BASE_DIR}/output_iso/modified_blos.iso", help="Path to a vanilla Twilight Princess ISO to use as a base.")
parser.add_argument("mode", choices=["default", "build_from_json"], nargs="?", default="default",
                    help="default | build from specified json")
args = parser.parse_args()

if not os.path.exists(args.modified_iso_path):
    iso_filepath = askopenfilename(
        title="Select ISO file (GZ2E01 ONLY)",
        filetypes=[("ISO files", "*.iso"), ("All files", "*.*")]
    )
    if not iso_filepath:
        print("No iso selected")
        exit()
else:
    iso_filepath = args.modified_iso_path

gcm = GCM(iso_filepath)
gcm.read_entire_disc()

# Create any wav files that need to come from base .ast files
ext_wav_names: List[str] = []
base_ast_data: List[BytesIO] = []
for json_file in json_folder:
    for track_data in json_file["tracks"]:
        for clip in track_data["clips"]:
            if clip.get("external_wav_name"):
                name = clip["external_wav_name"]
                if name not in ext_wav_names:
                    print(name)
                    ext_wav_names.append(clip["external_wav_name"])

for iso_path, gcm_file in gcm.files_by_path.items():
    if not iso_path.lower().endswith(".ast"):
        continue
    if not iso_path.startswith(f"files/Audiores/Stream/"):
        continue

    for filename in ext_wav_names:
        if not iso_path.startswith(f"files/Audiores/Stream/{filename}"):
            continue

        base_ast_data.append(gcm.read_file_data(iso_path))

ext_wav_names.sort()

for idx in range(len(base_ast_data)):
    with open(f"./ext_wavs/{ext_wav_names[idx]}.ast", "wb") as f:
        f.write(base_ast_data[idx].getvalue())
    subprocess.run(["ast_to_wav.exe", f"./ext_wavs/{ext_wav_names[idx]}.ast", f"./ext_wavs/{ext_wav_names[idx]}.wav"])

# Check if new wav files need to be made
json_to_use = []
create_wavs = True
for json_file in json_folder:
    for track_data in json_file["tracks"]:
        if track_data["new_track_name"] not in current_wav_files_stems and json_file not in json_to_use:
            create_wavs = True
            json_to_use.append(json_file)

# Create all necessary modified tracks based on user-provided wav files
for json_file in json_to_use:
    for wav in wav_files:
        if wav.stem == json_file["track_name"]:
            if create_wavs:
                wav_file = AudioSegment.from_wav(wav)
                synced_wav = sync(wav_file, json_file)

                for track_data in json_file["tracks"]:
                    print(track_data["new_track_name"])
                    new_wav = arrange_clips(synced_wav, track_data, ext_wav_folder)

                    # Silence
                    new_wav = add_silence_blocks(new_wav, track_data)

                    # Fade out
                    if track_data.get("fade_out"):
                        new_wav = new_wav.fade_out(track_data.get("fade_out", 0))
                    # Fade in
                    if track_data.get("fade_in"):
                        new_wav = new_wav.fade_in(track_data.get("fade_in", 0))

                    # Amplification
                    new_wav = new_wav + track_data.get("amplify", 0)
                    new_wav = add_amplify_blocks(new_wav, track_data)

                    # Sample Rate
                    new_wav = new_wav.set_frame_rate(track_data.get("sample_rate", 48000))

                    # Compression
                    AudioSegment.converter = ffmpeg.get_ffmpeg_exe()
                    path = ffmpeg.get_ffmpeg_exe()
                    new_wav.export(
                        f"output_wavs/{track_data["new_track_name"]}.wav",
                        format="wav",
                        parameters=[
                            "-loglevel", "quiet", 
                            "-af", f"acompressor=threshold={track_data.get("compressor_threshold", -1)}dB:ratio={track_data.get("compressor_ratio", 4)}:attack={track_data.get("compressor_attack", 5)}:release={track_data.get("compressor_release", 50)}"
                        ]
                    )
                        
# Handle adding all ast files to the game
# ast_names = []
# wav_folder = Path("output_wavs")
# wav_files = []
# for wav_file in wav_folder.glob("*.wav"):
#     wav_files.append(wav_file)
# stream_table_path = str()
# stream_table_data = BytesIO()

# for iso_path, gcm_file in gcm.files_by_path.items():
#     if iso_path.lower().endswith(".baa"):
#         stream_table_path = iso_path
#         stream_table_data = gcm.read_file_data(iso_path)
#         continue
#     if not iso_path.lower().endswith(".ast"):
#         continue
#     if not iso_path.startswith("files/Audiores/Stream/"):
#         continue

#     ast_name = Path(iso_path).stem
#     for wav in wav_files:
#         if ast_name == wav.stem:
#             with open(f"{wav_folder}/{wav.stem}.ast", "rb") as f:
#                 new_data = BytesIO(f.read())
#             ast_names.append(ast_name)
#             gcm.changed_files[iso_path] = new_data

# stream_table = BAA(stream_table_data)
# new_wavs: List[str] = []

# for wav in wav_files:
#     if wav.stem in ast_names:
#         continue
#     with open(f"{wav_folder}/{wav.stem}.ast", "rb") as f:
#         new_data = BytesIO(f.read())
#     gcm.add_new_file(f"files/Audiores/Stream/{wav.stem}.ast", new_data)
#     new_wavs.append(wav.stem)

# for name in new_wavs:
#     stream_table.add_new_ast(name)

# stream_table.write_new_baa(Path("new_table.baa"))
# with open(f"new_table.baa", "rb") as f:
#     new_data = BytesIO(f.read())
# gcm.changed_files[stream_table_path] = new_data

# for _ in gcm.export_disc_to_iso_with_changed_files(f"carcos_music_mod.iso"):
#     pass
print("Done")
