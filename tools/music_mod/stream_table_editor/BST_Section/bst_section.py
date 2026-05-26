from io import BytesIO
from typing import List

def bytes_to_int(data: bytes) -> int:
    return int.from_bytes(data, byteorder='big', signed=True)

class BST_Section:
    def __init__(self, data: BytesIO):
        self.bst_header = BST_Header(data)
        self.bst_main_tbl = BST_Main_Table(data)
        self.se_tbl = Section_Table(data)
        self.bgm_tbl = Section_Table(data)
        self.stream_tbl = Section_Table(data)
        # Param Tables
        self.se_param_tbls: List[Param_Table] = []
        for idx in range(bytes_to_int(self.se_tbl.num_entries)):
            self.se_param_tbls.insert(idx, Param_Table(data))
        self.bgm_param_tbl = Param_Table(data)
        self.stream_param_tbl = Param_Table(data)
        # Params
        self.bgm_params: List[BGM_Params] = []
        for idx in range(bytes_to_int(self.bgm_param_tbl.num_entries)):
            self.bgm_params.insert(idx, BGM_Params(data))
        num_of_param_blocks = bytes_to_int(self.se_tbl.num_entries)
        self.se_params = SE_Param_Blocks(data, num_of_param_blocks, self.se_param_tbls)
        num_of_stream_params = self.stream_param_tbl.actual_entries
        self.stream_params = Stream_Params(data, num_of_stream_params)
        self.bst_section_size = self.get_bst_section_size()
        self.bst_padding = b''
        while self.bst_section_size % 32 != 0:
            self.bst_padding += data.read(1)
            self.bst_section_size += 1

    def get_bst_section_size(self) -> int:
        size = int(32)  # BST Header size
        size += (bytes_to_int(self.bst_main_tbl.num_entries) + 1) * 4
        size += (bytes_to_int(self.se_tbl.num_entries) + 1) * 4
        size += (bytes_to_int(self.bgm_tbl.num_entries) + 1) * 4
        size += (bytes_to_int(self.stream_tbl.num_entries) + 1) * 4
        for tbl in self.se_param_tbls:
            size += (bytes_to_int(tbl.num_entries) + 2) * 4
        size += (bytes_to_int(self.bgm_param_tbl.num_entries) + 2) * 4
        size += (bytes_to_int(self.stream_param_tbl.num_entries) + 2) * 4
        size += bytes_to_int(self.bgm_param_tbl.num_entries) * 4
        size += self.se_params.get_size()
        size += self.stream_params.get_size()
        return size
    
    def add_ast_entry(self, filename: bytes):
        self.stream_param_tbl.add_entry()           # adds 4 bytes
        self.stream_params.add_entry(filename)      # adds (4 + len(ast_filepath)) bytes
        self.edit_offsets()
        self.recalculate_padding()
    
    def edit_offsets(self):
        # edit se parameter tables offsets
        for tbl in self.se_param_tbls:            
            for idx in range(len(tbl.param_offsets)):
                if tbl.param_offsets[idx].offset != b'\x00\x00\x00':
                    tbl.param_offsets[idx].offset = int(bytes_to_int(tbl.param_offsets[idx].offset) + 4).to_bytes(3)
        # edit bgm parameter table offsets
        for idx in range(len(self.bgm_param_tbl.param_offsets)):
            if self.bgm_param_tbl.param_offsets[idx].offset != b'\x00\x00\x00':
                self.bgm_param_tbl.param_offsets[idx].offset = int(bytes_to_int(self.bgm_param_tbl.param_offsets[idx].offset) + 4).to_bytes(3)
        # edit stream parameter table offsets
        for idx in range(len(self.stream_param_tbl.param_offsets)):
            if self.stream_param_tbl.param_offsets[idx].offset != b'\x00\x00\x00':
                self.stream_param_tbl.param_offsets[idx].offset = int(bytes_to_int(self.stream_param_tbl.param_offsets[idx].offset) + 4).to_bytes(3)
        # edit stream param filename offsets
        for idx in range(len(self.stream_params.filename_offset)):
            self.stream_params.filename_offset[idx] = int(bytes_to_int(self.stream_params.filename_offset[idx]) + 12).to_bytes(4)

    def recalculate_padding(self):
        self.bst_section_size = self.get_bst_section_size()
        self.bst_padding = b''
        while self.bst_section_size % 8 != 0:
            self.bst_padding += b'\x00'
            self.bst_section_size += 1
    
    def to_bytes(self):
        out = bytearray()
        out += self.bst_header.to_bytes()
        out += self.bst_main_tbl.to_bytes()
        out += self.se_tbl.to_bytes()
        out += self.bgm_tbl.to_bytes()
        out += self.stream_tbl.to_bytes()
        for tbl in self.se_param_tbls:
            out += tbl.to_bytes()
        out += self.bgm_param_tbl.to_bytes()
        out += self.stream_param_tbl.to_bytes()

        for params in self.bgm_params:
            out += params.to_bytes()
        out += self.se_params.to_bytes()
        num_of_stream_params = self.stream_param_tbl.actual_entries
        out += self.stream_params.to_bytes(num_of_stream_params)

        out += self.bst_padding
        return out

class BST_Header:
    def __init__(self, data: BytesIO):
        self.magic = data.read(4)
        self.unk1 = data.read(4)
        self.unk2 = data.read(1)
        self.unk3 = data.read(3)
        self.main_tbl_offset = data.read(4)
        self.padding = data.read(16)

    def to_bytes(self):
        out = bytearray()
        out += self.magic
        out += self.unk1
        out += self.unk2
        out += self.unk3
        out += self.main_tbl_offset
        out += self.padding
        return out

class BST_Main_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.se_tbl_offset = data.read(4)
        self.bgm_tbl_offset = data.read(4)
        self.stream_tbl_offset = data.read(4)

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        out += self.se_tbl_offset
        out += self.bgm_tbl_offset
        out += self.stream_tbl_offset
        return out
    
class Section_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.offsets: List[BytesIO] = []
        for idx in range(bytes_to_int(self.num_entries)):
            self.offsets.insert(idx, data.read(4))

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        for offset in self.offsets:
            out += offset
        return out

class Param_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.actual_entries = int(0)
        self.padding = data.read(4)
        self.param_offsets: List[Param_Offset] = []
        for idx in range(bytes_to_int(self.num_entries)):
            self.param_offsets.insert(idx, Param_Offset(data))
            if self.param_offsets[idx].offset != b'\x00\x00\x00':
                self.actual_entries += 1

    def add_entry(self):
        self.num_entries = int(bytes_to_int(self.num_entries) + 1).to_bytes(4)
        self.actual_entries += 1
        self.param_offsets.append(Param_Offset.add_new_stream_param_offset(self.param_offsets[bytes_to_int(self.num_entries) - 2].offset))
    

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        out += self.padding
        for param_offset in self.param_offsets:
            out += param_offset.to_bytes()
        return out

class Param_Offset:
    def __init__(self, data: BytesIO):
        self.unk = data.read(1)
        self.offset = data.read(3)

    @classmethod
    def add_new_stream_param_offset(cls, prev_offset: bytes):
        self = cls.__new__(cls)
        self.unk = b'\x71'
        self.offset = int(bytes_to_int(prev_offset) + 8).to_bytes(3)
        return self

    def to_bytes(self):
        out = bytearray()
        out += self.unk
        out += self.offset
        return out

class BGM_Params:
    def __init__(self, data: BytesIO):
        self.unk1 = data.read(1)
        self.unk2 = data.read(1)
        self.unk3 = data.read(1)
        self.unk4 = data.read(1)

    def to_bytes(self):
        out = bytearray()
        out += self.unk1
        out += self.unk2
        out += self.unk3
        out += self.unk4
        return out

class SE_Param_Blocks:
    def __init__(self, data: BytesIO, num_of_param_blocks: int, se_param_tbl: List[Param_Table]):
        self.blocks: List[SE_Params] = []
        self.num_of_params_per_block: List[int] = []
        for tbl in se_param_tbl:
            self.num_of_params_per_block.append(tbl.actual_entries)
        for idx in range(num_of_param_blocks):
            self.blocks.insert(idx, SE_Params(data, self.num_of_params_per_block[idx]))

    def get_size(self) -> int:
        size = int(0)
        for block in self.blocks:
            size += block.get_size()
        return size

    def to_bytes(self):
        out = bytearray()
        for block in self.blocks:
            out += block.to_bytes()
        return out

class SE_Params:
    def __init__(self, data: BytesIO, num_of_params_per_block: int):
        self.parameters: List[bytes] = []
        for idx in range(num_of_params_per_block):
            self.parameters.insert(idx, data.read(12))

    def get_size(self) -> int:
        return 12 * len(self.parameters)

    def to_bytes(self):
        out = bytearray()
        for param in self.parameters:
            out += param
        return out

class Stream_Params:
    def __init__(self, data: BytesIO, num_of_stream_params: int):
        self.unk1: List[bytes] = []
        self.volume: List[bytes] = []
        self.channel_count: List[bytes] = []
        self.filename_offset: List[bytes] = []
        for idx in range(num_of_stream_params):
            self.unk1.insert(idx, data.read(1))
            self.volume.insert(idx, data.read(1))
            self.channel_count.insert(idx, data.read(2))
            self.filename_offset.insert(idx, data.read(4))
        
        # Collect null-terminated filename list
        self.filenames: List[bytes] = []
        for idx in range(num_of_stream_params):
            if idx + 1 < num_of_stream_params:
                filename_length = bytes_to_int(self.filename_offset[idx + 1]) - bytes_to_int(self.filename_offset[idx])
                self.filenames.insert(idx, data.read(filename_length))
            else:
                # For the last offset, read the bytes until reaching the null terminator and add b'\x00' to the end
                self.filenames.insert(idx, b'')
                old_pos = data.tell()
                while data.read(1) != b'\x00':
                    data.seek(old_pos)
                    self.filenames[idx] += data.read(1)
                    old_pos = data.tell()
                self.filenames[idx] += b'\x00'

    def get_size(self) -> int:
        size = 8 * len(self.unk1)
        for name in self.filenames:
            size += len(name)
        return size
    
    def add_entry(self, filename: bytes):
        self.unk1.append(b'\x40')
        self.volume.append(b'\x7F')
        self.channel_count.append(b'\x00\x0E')
        prev_last_filename_offset = self.filename_offset.pop()
        self.filename_offset.append(prev_last_filename_offset)
        prev_last_filename = self.filenames.pop()
        self.filenames.append(prev_last_filename)
        self.filename_offset.append(int(bytes_to_int(prev_last_filename_offset) + len(prev_last_filename)).to_bytes(4))
        self.filenames.append(filename)


    def to_bytes(self, num_of_stream_params: int):
        out = bytearray()
        for idx in range(num_of_stream_params):
            out += self.unk1[idx]
            out += self.volume[idx]
            out += self.channel_count[idx]
            out += self.filename_offset[idx]
        for idx in range(num_of_stream_params):
            out += self.filenames[idx]
        return out
