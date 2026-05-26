from io import BytesIO
from typing import List
from pathlib import Path
from .BST_Section import BST_Section
from .BSTN_Section import BSTN_Section

def bytes_to_int(data: bytes) -> int:
    return int.from_bytes(data, byteorder='big', signed=True)

class BAA:
    def __init__(self, data: BytesIO):
        self.magic1 = data.read(4)
        self.magic2 = data.read(4)
        self.bst_start_offset = data.read(4)
        self.bst_end_offset = data.read(4)
        self.bstn_magic = data.read(4)
        self.bstn_start_offset = data.read(4)
        self.bstn_end_offset = data.read(4)
        # WSYS Headers
        self.wsys_headers: List[WSYS_Header] = [WSYS_Header(data), WSYS_Header(data)]
        # BNK Headers
        self.bnk_headers: List[BNK_Header] = [BNK_Header(data), BNK_Header(data), BNK_Header(data), BNK_Header(data),
                                              BNK_Header(data), BNK_Header(data), BNK_Header(data), BNK_Header(data)]
        self.bsc_magic = data.read(4)
        self.bsc_start_offset = data.read(4)
        self.bsc_end_offset = data.read(4)
        self.bfca_magic = data.read(4)
        self.bfca_start_offset = data.read(4)
        self.magic3 = data.read(4)

        # BST Section
        self.bst_section = BST_Section(data)

        # BSTN Section
        self.bstn_section = BSTN_Section(data)

        # WSYS Section
        lowest_ibnk_offset = bytes_to_int(self.bnk_headers[0].ibnk_offset)
        for bnk_header in self.bnk_headers:
            if bytes_to_int(bnk_header.ibnk_offset) < lowest_ibnk_offset:
                lowest_ibnk_offset = bytes_to_int(bnk_header.ibnk_offset)
        wsys_section_length = lowest_ibnk_offset - bytes_to_int(self.wsys_headers[0].offset)
        self.wsys_section = data.read(wsys_section_length)

        # IBNK Section
        self.ibnk_section = data.read(bytes_to_int(self.bsc_start_offset) - lowest_ibnk_offset)

        # BSC Section
        self.bsc_section = data.read(bytes_to_int(self.bsc_end_offset) - bytes_to_int(self.bsc_start_offset))

        # BFCA Section
        self.bfca_section = data.read()

    def add_new_ast(self, ast_name: str):
        ast_name_bytes = ast_name.encode()
        ast_filepath = b'AudioRes/Stream/' + ast_name_bytes + b'.ast\x00'
        self.bst_section.add_ast_entry(ast_filepath)
        self.bstn_section.add_ast_entry(ast_name)
        self.shift_section_offsets()

    def shift_section_offsets(self):
        # BST Section Shift
        self.bst_end_offset = int(bytes_to_int(self.bst_start_offset) + (self.bst_section.get_bst_section_size() + len(self.bst_section.bst_padding))).to_bytes(4)
        bst_size_change = bytes_to_int(self.bstn_start_offset)
        self.bstn_start_offset = self.bst_end_offset
        bst_size_change = bytes_to_int(self.bstn_start_offset) - bst_size_change

        # BSTN Section Shift
        bstn_size_change = bytes_to_int(self.bstn_end_offset)
        self.bstn_end_offset = int(bytes_to_int(self.bstn_start_offset) + self.bstn_section.bstn_section_size).to_bytes(4)
        bstn_size_change = bytes_to_int(self.bstn_end_offset) - bstn_size_change

        # WSYS Section Shift
        wsys_section_one_length = bytes_to_int(self.wsys_headers[1].offset) - bytes_to_int(self.wsys_headers[0].offset)
        wsys_section_two_length = bytes_to_int(self.bnk_headers[0].ibnk_offset) - bytes_to_int(self.wsys_headers[1].offset)
        self.wsys_headers[0].offset = self.bstn_end_offset
        self.wsys_headers[1].offset = int(bytes_to_int(self.wsys_headers[0].offset) + wsys_section_one_length).to_bytes(4)

        # BNK Section Shift
        original_offset = bytes_to_int(self.bnk_headers[0].ibnk_offset)
        self.bnk_headers[0].ibnk_offset = int(bytes_to_int(self.wsys_headers[1].offset) + wsys_section_two_length).to_bytes(4)
        section_shift = bytes_to_int(self.bnk_headers[0].ibnk_offset) - original_offset
        for idx in range(len(self.bnk_headers)):
            if idx == 0:
                continue
            self.bnk_headers[idx].ibnk_offset = int(bytes_to_int(self.bnk_headers[idx].ibnk_offset) + section_shift).to_bytes(4)

        # BSC Section Shift
        self.bsc_start_offset = int(bytes_to_int(self.bsc_start_offset) + section_shift).to_bytes(4)
        self.bsc_end_offset = int(bytes_to_int(self.bsc_end_offset) + section_shift).to_bytes(4)

        # BFCA Section Shift
        self.bfca_start_offset = int(bytes_to_int(self.bfca_start_offset) + section_shift).to_bytes(4)

    def to_bytes(self):
        out = bytearray()
        out += self.magic1
        out += self.magic2
        out += self.bst_start_offset
        out += self.bst_end_offset
        out += self.bstn_magic
        out += self.bstn_start_offset
        out += self.bstn_end_offset
        for header in self.wsys_headers:
            out += header.to_bytes()
        for header in self.bnk_headers:
            out += header.to_bytes()
        out += self.bsc_magic
        out += self.bsc_start_offset
        out += self.bsc_end_offset
        out += self.bfca_magic
        out += self.bfca_start_offset
        out += self.magic3

        # BST Section
        out += self.bst_section.to_bytes()

        # BSTN Section
        out += self.bstn_section.to_bytes()

        out += self.wsys_section
        out += self.ibnk_section
        out += self.bsc_section
        out += self.bfca_section
        return out
    
    def write_new_baa(self, path: Path):
        data = self.to_bytes()
        with open(path, "wb") as f:
            f.write(data)


class WSYS_Header:
    def __init__(self, data: BytesIO):
        self.magic = data.read(4)
        self.id = data.read(4)
        self.offset = data.read(4)
        self.bitfield = data.read(4)
    
    def to_bytes(self):
        out = bytearray()
        out += self.magic
        out += self.id
        out += self.offset
        out += self.bitfield
        return out


class BNK_Header:
    def __init__(self, data: BytesIO):
        self.magic = data.read(4)
        self.ibnk_id = data.read(4)
        self.ibnk_offset = data.read(4)

    def to_bytes(self):
        out = bytearray()
        out += self.magic
        out += self.ibnk_id
        out += self.ibnk_offset
        return out
