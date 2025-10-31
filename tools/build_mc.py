#!/usr/bin/env python3
"""
SPLATSTORM X - Memory Card Image Builder
Creates complete .PS2 boot image with assets for Memory Card deployment
"""

import os
import struct
import shutil
import json
from pathlib import Path
from datetime import datetime

class PS2ImageBuilder:
    def __init__(self):
        self.sector_size = 2048
        self.max_filename_length = 32
        
    def create_ps2_image(self, elf_path, assets_dir, output_path, title="SPLATSTORM X"):
        """Create a complete PS2 Memory Card boot image"""
        
        print(f"Creating PS2 Memory Card image: {output_path}")
        print(f"ELF: {elf_path}")
        print(f"Assets: {assets_dir}")
        
        # Create temporary directory structure
        temp_dir = Path("temp_ps2_build")
        temp_dir.mkdir(exist_ok=True)
        
        try:
            # Prepare all files
            files_to_pack = self._prepare_files(elf_path, assets_dir, temp_dir, title)
            
            # Calculate total size
            total_size = sum(self._get_padded_size(f[1]) for f in files_to_pack)
            print(f"Total image size: {total_size / 1024:.1f} KB ({total_size / (1024*1024):.2f} MB)")
            
            # Create the PS2 image
            self._write_ps2_image(files_to_pack, output_path)
            
            # Verify the image
            self._verify_ps2_image(output_path, files_to_pack)
            
            print(f"PS2 image created successfully: {output_path}")
            
        finally:
            # Clean up temp directory
            shutil.rmtree(temp_dir, ignore_errors=True)
    
    def _prepare_files(self, elf_path, assets_dir, temp_dir, title):
        """Prepare all files for packaging"""
        files_to_pack = []
        
        # 1. SYSTEM.CNF (boot configuration)
        system_cnf = temp_dir / "SYSTEM.CNF"
        self._create_system_cnf(system_cnf, title)
        files_to_pack.append(("SYSTEM.CNF", system_cnf))
        
        # 2. Main ELF executable
        if Path(elf_path).exists():
            elf_dest = temp_dir / "SPLATSTORM_X.ELF"
            shutil.copy2(elf_path, elf_dest)
            files_to_pack.append(("SPLATSTORM_X.ELF", elf_dest))
        else:
            print(f"Warning: ELF file not found: {elf_path}")
        
        # 3. Icon file
        icon_path = temp_dir / "icon.ico"
        self._create_ps2_icon(icon_path, title)
        files_to_pack.append(("icon.ico", icon_path))
        
        # 4. Assets directory
        assets_path = Path(assets_dir)
        if assets_path.exists():
            for asset_file in assets_path.rglob('*'):
                if asset_file.is_file():
                    # Copy to temp directory with relative path
                    rel_path = asset_file.relative_to(assets_path)
                    dest_path = temp_dir / "assets" / rel_path
                    dest_path.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(asset_file, dest_path)
                    
                    # Add to pack list
                    pack_name = f"assets/{rel_path}".replace('\\', '/')
                    files_to_pack.append((pack_name, dest_path))
        
        # 5. IOP modules (if they exist)
        iop_dir = Path("iop")
        if iop_dir.exists():
            for irx_file in iop_dir.glob("*.irx"):
                if irx_file.is_file():
                    dest_path = temp_dir / "iop" / irx_file.name
                    dest_path.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(irx_file, dest_path)
                    files_to_pack.append((f"iop/{irx_file.name}", dest_path))
        
        # 6. Build info
        build_info = temp_dir / "build_info.txt"
        self._create_build_info(build_info)
        files_to_pack.append(("build_info.txt", build_info))
        
        return files_to_pack
    
    def _create_system_cnf(self, system_cnf_path, title):
        """Create SYSTEM.CNF boot configuration"""
        with open(system_cnf_path, 'w') as f:
            f.write("BOOT2 = cdrom0:\\SPLATSTORM_X.ELF;1\n")
            f.write("VER = 1.00\n")
            f.write("VMODE = PAL\n")
            f.write(f"TITLE0 = {title}\n")
            f.write("TITLE1 = Gaussian Splat Engine\n")
    
    def _create_ps2_icon(self, icon_path, title):
        """Create a PS2 icon file with proper format"""
        
        # PS2 icon is 128x128 pixels, 16-bit color
        icon_width = 128
        icon_height = 128
        
        with open(icon_path, 'wb') as f:
            # Icon header
            f.write(b'PS2I')  # Magic number
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<I', icon_width))  # Width
            f.write(struct.pack('<I', icon_height))  # Height
            f.write(title.encode('ascii').ljust(64, b'\x00'))  # Title
            
            # Generate icon data (simple SPLATSTORM pattern)
            for y in range(icon_height):
                for x in range(icon_width):
                    # Create a splat-like pattern
                    center_x, center_y = icon_width // 2, icon_height // 2
                    dx, dy = x - center_x, y - center_y
                    dist = (dx*dx + dy*dy) ** 0.5
                    
                    # Gaussian-like falloff
                    intensity = max(0, 1.0 - (dist / (icon_width * 0.4)))
                    
                    # Color based on position and intensity
                    r = int(intensity * 255 * (0.8 + 0.2 * (x / icon_width)))
                    g = int(intensity * 255 * (0.6 + 0.4 * (y / icon_height)))
                    b = int(intensity * 255 * 0.9)
                    
                    # Convert to 16-bit RGB565
                    r16 = (r >> 3) & 0x1F
                    g16 = (g >> 2) & 0x3F
                    b16 = (b >> 3) & 0x1F
                    pixel = (r16 << 11) | (g16 << 5) | b16
                    
                    f.write(struct.pack('<H', pixel))
    
    def _create_build_info(self, build_info_path):
        """Create build information file"""
        with open(build_info_path, 'w') as f:
            f.write("SPLATSTORM X - Build Information\n")
            f.write("=" * 40 + "\n")
            f.write(f"Build Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"Version: 1.0.0\n")
            f.write(f"Target: PlayStation 2\n")
            f.write(f"Engine: Gaussian Splat Renderer\n")
            f.write(f"Max Splats: 100,000\n")
            f.write(f"Target FPS: 120+\n")
            f.write("\nMemory Layout:\n")
            f.write(f"  EE Code Base: 0x{0x00010000:08X}\n")
            f.write(f"  DMA Buffers:  0x{0x00100000:08X}\n")
            f.write(f"  VRAM Atlas:   0x{0x11000000:08X}\n")
            f.write(f"  VRAM Octree:  0x{0x11010000:08X}\n")
    
    def _get_padded_size(self, file_path):
        """Get file size padded to sector boundary"""
        size = file_path.stat().st_size
        return ((size + self.sector_size - 1) // self.sector_size) * self.sector_size
    
    def _write_ps2_image(self, files_to_pack, output_path):
        """Write the complete PS2 image"""
        with open(output_path, 'wb') as ps2_file:
            # Write directory header
            self._write_directory_header(ps2_file, files_to_pack)
            
            # Write file data
            current_offset = self._get_directory_size(files_to_pack)
            
            for name, file_path in files_to_pack:
                print(f"  Adding: {name} ({file_path.stat().st_size} bytes)")
                
                # Write file data
                with open(file_path, 'rb') as f:
                    data = f.read()
                    ps2_file.write(data)
                    
                    # Pad to sector boundary
                    padding = (self.sector_size - (len(data) % self.sector_size)) % self.sector_size
                    if padding > 0:
                        ps2_file.write(b'\x00' * padding)
                
                current_offset += self._get_padded_size(file_path)
    
    def _write_directory_header(self, ps2_file, files_to_pack):
        """Write directory structure header"""
        # Simple directory format
        ps2_file.write(b'PS2D')  # Directory magic
        ps2_file.write(struct.pack('<I', len(files_to_pack)))  # File count
        
        current_offset = self._get_directory_size(files_to_pack)
        
        for name, file_path in files_to_pack:
            # File entry
            name_bytes = name.encode('ascii')[:self.max_filename_length-1]
            name_padded = name_bytes.ljust(self.max_filename_length, b'\x00')
            
            ps2_file.write(name_padded)  # Filename
            ps2_file.write(struct.pack('<I', current_offset))  # Offset
            ps2_file.write(struct.pack('<I', file_path.stat().st_size))  # Size
            ps2_file.write(struct.pack('<I', 0))  # Flags/attributes
            
            current_offset += self._get_padded_size(file_path)
        
        # Pad directory to sector boundary
        dir_size = ps2_file.tell()
        padding = (self.sector_size - (dir_size % self.sector_size)) % self.sector_size
        if padding > 0:
            ps2_file.write(b'\x00' * padding)
    
    def _get_directory_size(self, files_to_pack):
        """Calculate directory header size"""
        header_size = 8  # Magic + file count
        entry_size = self.max_filename_length + 12  # Name + offset + size + flags
        total_size = header_size + (len(files_to_pack) * entry_size)
        
        # Pad to sector boundary
        return ((total_size + self.sector_size - 1) // self.sector_size) * self.sector_size
    
    def _verify_ps2_image(self, output_path, files_to_pack):
        """Verify the created PS2 image"""
        print("Verifying PS2 image...")
        
        image_size = Path(output_path).stat().st_size
        expected_size = self._get_directory_size(files_to_pack)
        expected_size += sum(self._get_padded_size(f[1]) for f in files_to_pack)
        
        if image_size == expected_size:
            print(f"✓ Image size verification passed: {image_size} bytes")
        else:
            print(f"✗ Image size mismatch: {image_size} != {expected_size}")
        
        # Check if image fits on standard Memory Card
        mc_size_8mb = 8 * 1024 * 1024
        if image_size <= mc_size_8mb:
            print(f"✓ Image fits on 8MB Memory Card ({image_size / (1024*1024):.2f} MB used)")
        else:
            print(f"✗ Image too large for 8MB Memory Card ({image_size / (1024*1024):.2f} MB)")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='SPLATSTORM X Memory Card Image Builder')
    parser.add_argument('elf_file', help='Input ELF executable')
    parser.add_argument('-a', '--assets', default='assets', help='Assets directory')
    parser.add_argument('-o', '--output', default='SPLATSTORM_X.PS2', help='Output PS2 image')
    parser.add_argument('-t', '--title', default='SPLATSTORM X', help='Game title')
    
    args = parser.parse_args()
    
    builder = PS2ImageBuilder()
    builder.create_ps2_image(args.elf_file, args.assets, args.output, args.title)

if __name__ == "__main__":
    main()