from io import BytesIO
from typing import List

def bytes_to_int(data: bytes) -> int:
    return int.from_bytes(data, byteorder='big', signed=True)

class BSTN_Section:
    def __init__(self, data: BytesIO):
        self.bstn_section_header = BSTN_Header(data)
        self.bstn_main_tbl = BSTN_Main_Table(data)
        self.se_names_offsets_tbl = Names_Offsets_Table(data)
        self.bgm_names_offsets_tbl = Names_Offsets_Table(data)
        self.stream_names_offsets_tbl = Names_Offsets_Table(data)
        self.se_name_tbls: List[Name_Table] = []
        for idx in range(bytes_to_int(self.se_names_offsets_tbl.num_entries)):
            self.se_name_tbls.insert(idx, Name_Table(data))
        self.bgm_name_tbls: List[Name_Table] = []
        for idx in range(bytes_to_int(self.bgm_names_offsets_tbl.num_entries)):
            self.bgm_name_tbls.insert(idx, Name_Table(data))
        self.stream_name_tbls: List[Name_Table] = []
        for idx in range(bytes_to_int(self.stream_names_offsets_tbl.num_entries)):
            self.stream_name_tbls.insert(idx, Name_Table(data))
        self.se_names = Names_Section(data, self.se_names_offsets_tbl, self.se_name_tbls)
        self.bgm_names = Names_Section(data, self.bgm_names_offsets_tbl, self.bgm_name_tbls)
        self.stream_names = Names_Section(data, self.stream_names_offsets_tbl, self.stream_name_tbls)
        self.bstn_section_size = self.get_bstn_section_size()
        self.bstn_padding = b''
        while self.bstn_section_size % 32 != 0:
            self.bstn_padding += data.read(1)
            self.bstn_section_size += 1

    def get_bstn_section_size(self) -> int:
        size = int(32)  # BSTN Header size
        size += (bytes_to_int(self.bstn_main_tbl.num_entries) + 1) * 4
        size += (bytes_to_int(self.se_names_offsets_tbl.num_entries) + 2) * 4
        size += (bytes_to_int(self.bgm_names_offsets_tbl.num_entries) + 2) * 4
        size += (bytes_to_int(self.stream_names_offsets_tbl.num_entries) + 2) * 4
        for name_tbl in self.se_name_tbls:
            size += (bytes_to_int(name_tbl.num_entries) + 2) * 4
        for name_tbl in self.bgm_name_tbls:
            size += (bytes_to_int(name_tbl.num_entries) + 2) * 4
        for name_tbl in self.stream_name_tbls:
            size += (bytes_to_int(name_tbl.num_entries) + 2) * 4
        size += self.se_names.get_size()
        size += self.bgm_names.get_size()
        size += self.stream_names.get_size()
        return size
    
    def add_ast_entry(self, filename: str):
        # edit global name offsets
        self.se_names_offsets_tbl.global_name_offset = int(bytes_to_int(self.se_names_offsets_tbl.global_name_offset) + 4).to_bytes(4)
        self.bgm_names_offsets_tbl.global_name_offset = int(bytes_to_int(self.bgm_names_offsets_tbl.global_name_offset) + 4).to_bytes(4)
        self.stream_names_offsets_tbl.global_name_offset = int(bytes_to_int(self.stream_names_offsets_tbl.global_name_offset) + 4).to_bytes(4)

        for tbl in self.se_name_tbls:
            tbl.category_name_offset = int(bytes_to_int(tbl.category_name_offset) + 4).to_bytes(4)
            for idx in range(len(tbl.offsets)):
                tbl.offsets[idx] = int(bytes_to_int(tbl.offsets[idx]) + 4).to_bytes(4)
        for tbl in self.bgm_name_tbls:
            tbl.category_name_offset = int(bytes_to_int(tbl.category_name_offset) + 4).to_bytes(4)
            for idx in range(len(tbl.offsets)):
                tbl.offsets[idx] = int(bytes_to_int(tbl.offsets[idx]) + 4).to_bytes(4)
        for tbl in self.stream_name_tbls:
            tbl.category_name_offset = int(bytes_to_int(tbl.category_name_offset) + 4).to_bytes(4)
            for idx in range(len(tbl.offsets)):
                tbl.offsets[idx] = int(bytes_to_int(tbl.offsets[idx]) + 4).to_bytes(4)
        
        prev_last_stream_name = self.stream_names.names[0].name_list.pop()
        self.stream_names.names[0].name_list.append(prev_last_stream_name)
        stream_name_str = filename.upper()
        stream_name_bytes = b'Z2STRM_' + stream_name_str.encode() + b'\x00'
        self.stream_name_tbls[0].add_stream_entry(len(prev_last_stream_name))
        self.stream_names.add_stream_name(stream_name_bytes)

        # recalculate padding
        self.recalculate_padding()
    
    def recalculate_padding(self):
        self.bstn_section_size = self.get_bstn_section_size()
        self.bstn_padding = b''
        while self.bstn_section_size % 8 != 0:
            self.bstn_padding += b'\x00'
            self.bstn_section_size += 1

    def to_bytes(self):
        out = bytearray()
        out += self.bstn_section_header.to_bytes()
        out += self.bstn_main_tbl.to_bytes()
        out += self.se_names_offsets_tbl.to_bytes()
        out += self.bgm_names_offsets_tbl.to_bytes()
        out += self.stream_names_offsets_tbl.to_bytes()
        for tbl in self.se_name_tbls:
            out += tbl.to_bytes()
        for tbl in self.bgm_name_tbls:
            out += tbl.to_bytes()
        for tbl in self.stream_name_tbls:
            out += tbl.to_bytes()
        out += self.se_names.to_bytes()
        out += self.bgm_names.to_bytes()
        out += self.stream_names.to_bytes()
        out += self.bstn_padding
        return out

class BSTN_Header:
    def __init__(self, data: BytesIO):
        self.magic = data.read(4)
        self.unk1 = data.read(4)
        self.unk2 = data.read(1)
        self.unk3 = data.read(3)
        self.bstn_main_offset = data.read(4)
        self.padding = data.read(16)

    def to_bytes(self):
        out = bytearray()
        out += self.magic
        out += self.unk1
        out += self.unk2
        out += self.unk3
        out += self.bstn_main_offset
        out += self.padding
        return out

class BSTN_Main_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.se_names_tbl_offset = data.read(4)
        self.bgm_names_tbl_offset = data.read(4)
        self.stream_names_tbl_offset = data.read(4)

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        out += self.se_names_tbl_offset
        out += self.bgm_names_tbl_offset
        out += self.stream_names_tbl_offset
        return out
    
class Names_Offsets_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.global_name_offset = data.read(4)
        self.offsets: List[bytes] = []
        for idx in range(bytes_to_int(self.num_entries)):
            self.offsets.insert(idx, data.read(4))

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        out += self.global_name_offset
        for idx in range(bytes_to_int(self.num_entries)):
            out += self.offsets[idx]
        return out

class Name_Table:
    def __init__(self, data: BytesIO):
        self.num_entries = data.read(4)
        self.category_name_offset = data.read(4)
        self.offsets: List[bytes] = []
        for idx in range(bytes_to_int(self.num_entries)):
            self.offsets.insert(idx, data.read(4))

    def add_stream_entry(self, prev_last_name_length: int):
        # Shift all offsets up by four to account for last offset to be added to list
        for idx in range(len(self.offsets)):
            self.offsets[idx] = int(bytes_to_int(self.offsets[idx]) + 4).to_bytes(4)
        self.num_entries = int(bytes_to_int(self.num_entries) + 1).to_bytes(4)
        self.category_name_offset = int(bytes_to_int(self.category_name_offset) + 4).to_bytes(4)
        prev_last_offset = self.offsets.pop()
        self.offsets.append(prev_last_offset)
        self.offsets.append(int(bytes_to_int(prev_last_offset) + prev_last_name_length).to_bytes(4))

    def to_bytes(self):
        out = bytearray()
        out += self.num_entries
        out += self.category_name_offset
        for idx in range(bytes_to_int(self.num_entries)):
            out += self.offsets[idx]
        return out

class Names_Section:
    def __init__(self, data: BytesIO, name_offsets_tbl: Names_Offsets_Table, name_tbls: List[Name_Table]):
        self.global_name_length = bytes_to_int(name_tbls[0].category_name_offset) - bytes_to_int(name_offsets_tbl.global_name_offset)
        self.global_name = data.read(self.global_name_length)
        self.names: List[Names] = []
        for name_tbl in name_tbls:
            category_name_length = bytes_to_int(name_tbl.offsets[0]) - bytes_to_int(name_tbl.category_name_offset)
            self.names.append(Names(data, category_name_length, name_tbl))

    def get_size(self) -> int:
        size = self.global_name_length
        for name in self.names:
            size += name.get_size()
        return size
    
    def add_stream_name(self, name: bytes):
        self.names[0].add_name(name)

    def to_bytes(self):
        out = bytearray()
        out += self.global_name
        for name in self.names:
            out += name.to_bytes()
        return out

class Names:
    def __init__(self, data: BytesIO, category_name_length: int, name_tbl: Name_Table):
        self.category_name = data.read(category_name_length)
        self.name_list: List[bytes] = []
        for idx in range(bytes_to_int(name_tbl.num_entries)):
            if idx + 1 < bytes_to_int(name_tbl.num_entries):
                name_length = bytes_to_int(name_tbl.offsets[idx + 1]) - bytes_to_int(name_tbl.offsets[idx])
                self.name_list.insert(idx, data.read(name_length))
            else:
                # For the last offset, read the bytes until reaching the null terminator and add b'\x00' to the end
                self.name_list.insert(idx, b'')
                old_pos = data.tell()
                while data.read(1) != b'\x00':
                    data.seek(old_pos)
                    self.name_list[idx] += data.read(1)
                    old_pos = data.tell()
                self.name_list[idx] += b'\x00'

    def get_size(self) -> int:
        size = int(0)
        size += len(self.category_name)
        for name in self.name_list:
            size += len(name)
        return size
    
    def add_name(self, name: bytes):
        self.name_list.append(name)

    def to_bytes(self):
        out = bytearray()
        out += self.category_name
        for name in self.name_list:
            out += name
        return out
