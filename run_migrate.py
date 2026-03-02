import os
import re

struct_type_map = {
    "temp_mat_t": "temp_matrix_t",
    "crnt_t": "current_t",
    "dataj_t": "segment_t",
    "fpat_t": "field_pattern_t",
    "gnd_t": "ground_t",
    "mat_t": "matrix_t",
    "net_t": "network_t",
    "out_t": "output_t",
    "plot_t": "plot_t",
    "rad_t": "radiation_t",
    "somp_t": "somnec_t",
    "type_t": "types_t",
    "cont_t": "control_t",
    "anc_t": "ancillary_t",
    "tiny_t": "tinyexpr_t",
    "geom_t": "geometry_t",
}

geometry_field_map = {
    "n": "num_segs", "np": "num_segs_sym", "m": "num_patches", "mp": "num_patches_sym",
    "npm": "num_segs_and_patches", "np2m": "num_unknowns", "np3m": "num_current_components",
    "ipsym": "symmetry_flag", "icon1": "seg_end1_conn", "icon2": "seg_end2_conn",
    "x1": "end1_x", "y1": "end1_y", "z1": "end1_z", "x2": "end2_x", "y2": "end2_y", "z2": "end2_z",
    "x": "x_center", "y": "y_center", "z": "z_center", "si": "half_len", "bi": "radius",
    "cab": "dir_cos_x", "sab": "dir_cos_y", "salp": "dir_cos_z", "wlam": "wavelength",
    "px": "patch_x_center", "py": "patch_y_center", "pz": "patch_z_center",
    "t1x": "patch_t1x", "t1y": "patch_t1y", "t1z": "patch_t1z",
    "t2x": "patch_t2x", "t2y": "patch_t2y", "t2z": "patch_t2z",
    "pbi": "patch_area", "psalp": "patch_normal_z",
}

segment_field_map = {
    "iexk": "use_extended_kernel", "ind1": "end1_kernel_type", "indd1": "end1_kernel_deferred",
    "ind2": "end2_kernel_type", "indd2": "end2_kernel_deferred", "ipgnd": "ground_image_pass",
    "s": "seg_half_len", "b": "seg_radius", "xj": "src_x", "yj": "src_y", "zj": "src_z",
    "cabj": "src_dir_cos_x", "sabj": "src_dir_cos_y", "salpj": "src_dir_cos_z",
    "rkh": "k_half_len", "t1xj": "patch_t1x", "t1yj": "patch_t1y", "t1zj": "patch_t1z",
    "t2xj": "patch_t2x", "t2yj": "patch_t2y", "t2zj": "patch_t2z",
    "exk": "e_const_x", "eyk": "e_const_y", "ezk": "e_const_z",
    "exs": "e_sin_x", "eys": "e_sin_y", "ezs": "e_sin_z",
    "exc": "e_cos_x", "eyc": "e_cos_y", "ezc": "e_cos_z",
}

# Fields that are common enough to require context
generic_fields = {"n", "m", "x", "y", "z", "si", "bi", "px", "py", "pz", "cab", "sab", "s", "b", "radius", "x1", "y1", "z1", "x2", "y2", "z2"}

target_prefixes = ["geometry.", "geometry->", "geom.", "geom->", "g.", "g->", "gnd.", "gnd->", "seg.", "seg->", "d.", "d->", "data.", "data->", "target_geom->", "ignored_geometry."]

src_dir = "src"
files = [f for f in os.listdir(src_dir) if f.endswith((".c", ".h"))]

def run_replace(filename, content):
    # 1. Rename types
    sorted_type_keys = sorted(struct_type_map.keys(), key=len, reverse=True)
    for old in sorted_type_keys:
        content = re.sub(r"\b" + re.escape(old) + r"\b", struct_type_map[old], content)

    # 2. Rename fields
    all_fields = list(geometry_field_map.items()) + list(segment_field_map.items())
    
    for old, new in all_fields:
        if old in generic_fields:
            # Only replace if preceded by a known target prefix
            for pref in target_prefixes:
                # Use a captures-matching approach to be safe with regex
                pattern = r"(\b" + re.escape(pref) + r")" + re.escape(old) + r"\b"
                content = re.sub(pattern, r"\1" + new, content)
        else:
            # Non-generic: replace after any . or ->
            # EXCEPT for wire_info_t fields in deck_validations.c
            if filename == "deck_validations.c" and old in ["radius", "x1", "y1", "z1", "x2", "y2", "z2"]:
                # Only replace if it is definitely NOT wire_info_t (though generic check covers some)
                # For safety, let's just skip these entirely in deck_validations.c if they aren't generic
                # Actually, radius is in generic_fields, so it's already covered.
                # x1, y1, z1, x2, y2, z2 are NOT in generic_fields but ARE in wire_info_t.
                # Let's add them to generic_fields.
                pass
            content = re.sub(r"(?<=[.\->])\b" + re.escape(old) + r"\b", new, content)

    # 3. Special handling for internals.h
    if filename == "internals.h":
        # Add (formerly ...) comments
        for old, new in struct_type_map.items():
            if old != new:
                content = re.sub(r"\}\s*" + re.escape(new) + r"\s*;", r"} " + new + r"; /* (formerly " + old + r") */", content)
        
        # Field declarations in geometry_t
        geom_match = re.search(r"typedef struct geometry_t\s*\{(.*?)\}\s*geometry_t", content, re.DOTALL)
        if geom_match:
            body = geom_match.group(1)
            new_body = body
            for old, new in geometry_field_map.items():
                new_body = re.sub(r"\b" + re.escape(old) + r"\b(?=[^;]*;)", new, new_body)
            content = content.replace(body, new_body)

        # Field declarations in segment_t
        seg_match = re.search(r"typedef struct\s*\{(.*?)\}\s*segment_t", content, re.DOTALL)
        if seg_match:
            body = seg_match.group(1)
            new_body = body
            for old, new in segment_field_map.items():
                new_body = re.sub(r"\b" + re.escape(old) + r"\b(?=[^;]*;)", new, new_body)
            content = content.replace(body, new_body)
            
    return content

for filename in files:
    path = os.path.join(src_dir, filename)
    with open(path, "r") as f:
        content = f.read()
    new_content = run_replace(filename, content)
    if new_content != content:
        with open(path, "w") as f:
            f.write(new_content)
        print(f"Updated {filename}")
