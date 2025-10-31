#!/usr/bin/env python3
"""
SPLATSTORM X - Splat to Atlas Converter
Advanced asset pipeline with texture atlas generation and octree indexing
"""

import numpy as np
import struct
import argparse
from pathlib import Path
import json
import math

class SplatAtlasGenerator:
    def __init__(self):
        self.splats = []
        self.atlas_size = 1024
        self.atlas_layers = 4  # T0-T3
        self.octree_size = 512
        self.max_octree_depth = 8
        
    def load_ply_advanced(self, filename):
        """Load PLY with advanced parsing for Gaussian splat data"""
        print(f"Loading advanced PLY: {filename}")
        
        with open(filename, 'rb') as f:
            # Parse PLY header
            line = f.readline().decode('ascii').strip()
            if line != 'ply':
                raise ValueError("Not a valid PLY file")
            
            vertex_count = 0
            properties = []
            
            while True:
                line = f.readline().decode('ascii').strip()
                if line == 'end_header':
                    break
                elif line.startswith('element vertex'):
                    vertex_count = int(line.split()[2])
                elif line.startswith('property'):
                    parts = line.split()
                    prop_type = parts[1]
                    prop_name = parts[2]
                    properties.append((prop_type, prop_name))
            
            print(f"Found {vertex_count} vertices with {len(properties)} properties")
            
            # Read vertex data
            for i in range(vertex_count):
                vertex_data = {}
                
                # Read based on property types
                for prop_type, prop_name in properties:
                    if prop_type == 'float':
                        value = struct.unpack('f', f.read(4))[0]
                    elif prop_type == 'double':
                        value = struct.unpack('d', f.read(8))[0]
                    elif prop_type in ['uchar', 'uint8']:
                        value = struct.unpack('B', f.read(1))[0]
                    elif prop_type in ['int', 'int32']:
                        value = struct.unpack('i', f.read(4))[0]
                    else:
                        value = 0.0
                    
                    vertex_data[prop_name] = value
                
                # Convert to splat format
                splat = self._convert_vertex_to_splat(vertex_data)
                if splat:
                    self.splats.append(splat)
        
        print(f"Loaded {len(self.splats)} splats")
        
    def _convert_vertex_to_splat(self, vertex_data):
        """Convert vertex data to splat format"""
        try:
            # Position
            pos = [
                vertex_data.get('x', 0.0),
                vertex_data.get('y', 0.0), 
                vertex_data.get('z', 0.0)
            ]
            
            # Color (handle different formats)
            if 'red' in vertex_data:
                color = [
                    vertex_data['red'] / 255.0,
                    vertex_data['green'] / 255.0,
                    vertex_data['blue'] / 255.0
                ]
            elif 'r' in vertex_data:
                color = [vertex_data['r'], vertex_data['g'], vertex_data['b']]
            else:
                color = [1.0, 1.0, 1.0]
            
            # Alpha
            alpha = vertex_data.get('alpha', vertex_data.get('opacity', 1.0))
            if alpha > 1.0:
                alpha /= 255.0
            
            # Scale (for covariance)
            scale = [
                vertex_data.get('scale_0', vertex_data.get('scale_x', 0.1)),
                vertex_data.get('scale_1', vertex_data.get('scale_y', 0.1)),
                vertex_data.get('scale_2', vertex_data.get('scale_z', 0.1))
            ]
            
            # Rotation (quaternion to covariance)
            rot = [
                vertex_data.get('rot_0', vertex_data.get('qx', 0.0)),
                vertex_data.get('rot_1', vertex_data.get('qy', 0.0)),
                vertex_data.get('rot_2', vertex_data.get('qz', 0.0)),
                vertex_data.get('rot_3', vertex_data.get('qw', 1.0))
            ]
            
            # Compute covariance matrix from scale and rotation
            covariance = self._compute_covariance_from_scale_rot(scale, rot)
            
            return {
                'pos': pos,
                'color': color,
                'alpha': alpha,
                'covariance': covariance,
                'scale': scale,
                'rotation': rot
            }
            
        except Exception as e:
            print(f"Warning: Failed to convert vertex: {e}")
            return None
    
    def _compute_covariance_from_scale_rot(self, scale, rot):
        """Compute 3x3 covariance matrix from scale and rotation quaternion"""
        # Normalize quaternion
        qx, qy, qz, qw = rot
        norm = math.sqrt(qx*qx + qy*qy + qz*qz + qw*qw)
        if norm > 0:
            qx, qy, qz, qw = qx/norm, qy/norm, qz/norm, qw/norm
        
        # Convert quaternion to rotation matrix
        R = np.array([
            [1-2*(qy*qy+qz*qz), 2*(qx*qy-qz*qw), 2*(qx*qz+qy*qw)],
            [2*(qx*qy+qz*qw), 1-2*(qx*qx+qz*qz), 2*(qy*qz-qx*qw)],
            [2*(qx*qz-qy*qw), 2*(qy*qz+qx*qw), 1-2*(qx*qx+qy*qy)]
        ])
        
        # Scale matrix
        S = np.diag(scale)
        
        # Covariance = R * S * S^T * R^T
        RS = np.dot(R, S)
        cov = np.dot(RS, RS.T)
        
        # Return upper triangle (6 values)
        return [cov[0,0], cov[0,1], cov[0,2], cov[1,1], cov[1,2], cov[2,2]]
    
    def generate_texture_atlas(self):
        """Generate texture atlas from splat data"""
        print("Generating texture atlas...")
        
        # Create atlas layers
        atlases = []
        for layer in range(self.atlas_layers):
            atlas = np.zeros((self.atlas_size, self.atlas_size, 4), dtype=np.uint8)
            atlases.append(atlas)
        
        # Sort splats by size for better packing
        sorted_splats = sorted(self.splats, key=lambda s: max(s['scale']), reverse=True)
        
        # Pack splats into atlas
        packed_count = 0
        for i, splat in enumerate(sorted_splats):
            layer = i % self.atlas_layers
            
            # Calculate splat size in pixels
            splat_size = max(8, min(64, int(max(splat['scale']) * 100)))
            
            # Find position in atlas (simple grid packing)
            grid_size = self.atlas_size // splat_size
            grid_x = (i // self.atlas_layers) % grid_size
            grid_y = ((i // self.atlas_layers) // grid_size) % grid_size
            
            if grid_y * splat_size + splat_size <= self.atlas_size:
                x = grid_x * splat_size
                y = grid_y * splat_size
                
                # Render splat to atlas
                self._render_splat_to_atlas(atlases[layer], x, y, splat_size, splat)
                
                # Store atlas coordinates in splat
                splat['atlas_layer'] = layer
                splat['atlas_u'] = x / self.atlas_size
                splat['atlas_v'] = y / self.atlas_size
                splat['atlas_size'] = splat_size / self.atlas_size
                
                packed_count += 1
        
        print(f"Packed {packed_count}/{len(self.splats)} splats into atlas")
        return atlases
    
    def _render_splat_to_atlas(self, atlas, x, y, size, splat):
        """Render a single splat to the atlas"""
        # Create Gaussian kernel
        center = size // 2
        sigma = size / 6.0  # 3-sigma rule
        
        for dy in range(size):
            for dx in range(size):
                # Gaussian falloff
                dist_sq = (dx - center)**2 + (dy - center)**2
                weight = math.exp(-dist_sq / (2 * sigma**2))
                
                # Apply color and alpha
                color = [int(c * 255) for c in splat['color']]
                alpha = int(splat['alpha'] * weight * 255)
                
                atlas_x = x + dx
                atlas_y = y + dy
                
                if atlas_x < self.atlas_size and atlas_y < self.atlas_size:
                    atlas[atlas_y, atlas_x] = [color[0], color[1], color[2], alpha]
    
    def build_octree_index(self):
        """Build spatial octree index for culling"""
        print("Building octree index...")
        
        if not self.splats:
            return None
        
        # Find bounding box
        positions = [s['pos'] for s in self.splats]
        min_pos = np.min(positions, axis=0)
        max_pos = np.max(positions, axis=0)
        
        # Expand bounds slightly
        center = (min_pos + max_pos) / 2
        size = np.max(max_pos - min_pos) * 1.1
        min_pos = center - size/2
        max_pos = center + size/2
        
        # Build octree recursively
        root = self._build_octree_recursive(
            list(range(len(self.splats))), min_pos, max_pos, 0
        )
        
        # Convert to linear array for PS2
        octree_array = []
        self._flatten_octree(root, octree_array)
        
        print(f"Built octree with {len(octree_array)} nodes")
        return octree_array
    
    def _build_octree_recursive(self, splat_indices, min_pos, max_pos, depth):
        """Recursively build octree node"""
        if depth >= self.max_octree_depth or len(splat_indices) <= 8:
            # Leaf node
            return {
                'is_leaf': True,
                'splat_indices': splat_indices[:8],  # Max 8 per leaf
                'bounds': [min_pos[0], min_pos[1], min_pos[2], 
                          max_pos[0], max_pos[1], max_pos[2]]
            }
        
        # Split into 8 octants
        center = (min_pos + max_pos) / 2
        children = []
        
        for octant in range(8):
            # Calculate octant bounds
            oct_min = min_pos.copy()
            oct_max = max_pos.copy()
            
            if octant & 1: oct_min[0] = center[0]
            else:          oct_max[0] = center[0]
            if octant & 2: oct_min[1] = center[1]
            else:          oct_max[1] = center[1]
            if octant & 4: oct_min[2] = center[2]
            else:          oct_max[2] = center[2]
            
            # Find splats in this octant
            octant_splats = []
            for idx in splat_indices:
                pos = self.splats[idx]['pos']
                if (pos[0] >= oct_min[0] and pos[0] <= oct_max[0] and
                    pos[1] >= oct_min[1] and pos[1] <= oct_max[1] and
                    pos[2] >= oct_min[2] and pos[2] <= oct_max[2]):
                    octant_splats.append(idx)
            
            if octant_splats:
                child = self._build_octree_recursive(octant_splats, oct_min, oct_max, depth + 1)
                children.append(child)
            else:
                children.append(None)
        
        return {
            'is_leaf': False,
            'children': children,
            'bounds': [min_pos[0], min_pos[1], min_pos[2], 
                      max_pos[0], max_pos[1], max_pos[2]]
        }
    
    def _flatten_octree(self, node, array):
        """Flatten octree to linear array"""
        if not node:
            return -1
        
        node_index = len(array)
        
        if node['is_leaf']:
            # Leaf node
            array.append({
                'type': 'leaf',
                'splat_count': len(node['splat_indices']),
                'splat_indices': node['splat_indices'] + [0] * (8 - len(node['splat_indices'])),
                'bounds': node['bounds']
            })
        else:
            # Internal node - add placeholder
            array.append({
                'type': 'internal',
                'child_offsets': [0] * 8,
                'bounds': node['bounds']
            })
            
            # Add children and update offsets
            for i, child in enumerate(node['children']):
                if child:
                    child_index = self._flatten_octree(child, array)
                    array[node_index]['child_offsets'][i] = child_index
                else:
                    array[node_index]['child_offsets'][i] = -1
        
        return node_index
    
    def export_assets(self, output_dir):
        """Export all assets for PS2"""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        # Generate atlas
        atlases = self.generate_texture_atlas()
        
        # Export atlas textures
        for i, atlas in enumerate(atlases):
            atlas_file = output_path / f"atlas_t{i}.raw"
            with open(atlas_file, 'wb') as f:
                # Convert to PS2 format (RGBA32)
                for y in range(self.atlas_size):
                    for x in range(self.atlas_size):
                        pixel = atlas[y, x]
                        f.write(struct.pack('BBBB', pixel[0], pixel[1], pixel[2], pixel[3]))
            print(f"Exported atlas layer {i}: {atlas_file}")
        
        # Build and export octree
        octree = self.build_octree_index()
        if octree:
            octree_file = output_path / "octree.idx"
            with open(octree_file, 'wb') as f:
                # Header
                f.write(struct.pack('I', len(octree)))  # Node count
                f.write(struct.pack('I', self.max_octree_depth))  # Max depth
                
                # Nodes
                for node in octree:
                    if node['type'] == 'leaf':
                        f.write(struct.pack('I', 1))  # Leaf flag
                        f.write(struct.pack('I', node['splat_count']))
                        for idx in node['splat_indices']:
                            f.write(struct.pack('I', idx))
                    else:
                        f.write(struct.pack('I', 0))  # Internal flag
                        f.write(struct.pack('I', 0))  # Padding
                        for offset in node['child_offsets']:
                            f.write(struct.pack('i', offset))
                    
                    # Bounds (6 floats)
                    for bound in node['bounds']:
                        f.write(struct.pack('f', bound))
            
            print(f"Exported octree: {octree_file}")
        
        # Export splat data
        splat_file = output_path / "scene.splat"
        with open(splat_file, 'wb') as f:
            # Header
            f.write(struct.pack('I', len(self.splats)))
            f.write(struct.pack('I', 0x53504C54))  # 'SPLT' magic
            
            # Splat data
            for splat in self.splats:
                # Position (12 bytes)
                f.write(struct.pack('fff', *splat['pos']))
                f.write(struct.pack('f', 0))  # Padding
                
                # Covariance (24 bytes)
                f.write(struct.pack('ffffff', *splat['covariance']))
                f.write(struct.pack('ff', 0, 0))  # Padding
                
                # Color + Alpha (16 bytes)
                f.write(struct.pack('ffff', *splat['color'], splat['alpha']))
                
                # Atlas coordinates (16 bytes)
                f.write(struct.pack('ffff', 
                    splat.get('atlas_u', 0),
                    splat.get('atlas_v', 0),
                    splat.get('atlas_size', 0),
                    splat.get('atlas_layer', 0)))
        
        print(f"Exported splats: {splat_file}")
        
        # Export metadata
        metadata = {
            'splat_count': len(self.splats),
            'atlas_size': self.atlas_size,
            'atlas_layers': self.atlas_layers,
            'octree_nodes': len(octree) if octree else 0,
            'octree_depth': self.max_octree_depth
        }
        
        metadata_file = output_path / "metadata.json"
        with open(metadata_file, 'w') as f:
            json.dump(metadata, f, indent=2)
        
        print(f"Exported metadata: {metadata_file}")
        print(f"Asset export complete: {len(self.splats)} splats, {self.atlas_layers} atlas layers")

def main():
    parser = argparse.ArgumentParser(description='SPLATSTORM X Advanced Asset Pipeline')
    parser.add_argument('input', help='Input PLY file')
    parser.add_argument('-o', '--output', default='assets', help='Output directory')
    parser.add_argument('--atlas-size', type=int, default=1024, help='Atlas texture size')
    parser.add_argument('--atlas-layers', type=int, default=4, help='Number of atlas layers')
    parser.add_argument('--octree-depth', type=int, default=8, help='Maximum octree depth')
    
    args = parser.parse_args()
    
    generator = SplatAtlasGenerator()
    generator.atlas_size = args.atlas_size
    generator.atlas_layers = args.atlas_layers
    generator.max_octree_depth = args.octree_depth
    
    generator.load_ply_advanced(args.input)
    generator.export_assets(args.output)

if __name__ == '__main__':
    main()