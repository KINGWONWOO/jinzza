"""bake_props.py - rescale/re-pivot the Tripo FBX props in this folder before importing them into Unreal.

Why: Tripo exports in metres and normalises every model to ~1 unit, so a straight UE import is ~1cm tall, and
StaticMeshTools.import_file has no scale option. Component scale is not an option either - held props attach with
SnapToTargetIncludingScale, which resets it. So the size is baked into the mesh here.

Run (Blender 4.3, headless):
  "C:/Program Files/Blender Foundation/Blender 4.3/blender.exe" --background --python bake_props.py -- <out_dir> [Name ...]
Then import each <out_dir>/<Name>.fbx with import_materials=False and assign the tripo_mat_* material to the slot
(materials/textures were created by the first, raw import - see Content/JINZZA/Props/Meshes/*).

Note: UE mirrors Blender's Y axis, so 'front(-Y)' in Blender is +Y in Unreal.
Sizes/pivots/rotations per model are in CFG below (cm; pivot = bbox centre to match the old primitive placeholders).
"""
"""Rotate / re-pivot / scale each Tripo FBX to its placeholder's in-game size and export a clean FBX.
Units: Blender 1 unit = 1 m, exported FBX default => 1 m = 100 UE cm. So size_cm/100 = Blender units."""
import bpy, math, mathutils, sys, os
D = "E:/UnrealProjectInE/jinzza/docs/modelFromTripo/"
OUT = sys.argv[sys.argv.index("--")+1]

# name: src, rot_deg (x,y,z, applied in that order), fit=(axis, size_cm) measured AFTER rotation, pivot
CFG = {
 "Bat":        dict(src=D+"wooden+baseball+bat+3d+model/tripo_convert_14b9f5a5-2507-4026-b0d8-4250352abd31.fbx",
                    rot=(-90,0,0), fit=('Z',100.0), pivot='center'),   # barrel (-Y in source) -> +Z, like the placeholder
 "Basketball": dict(src=D+"basketball+3d+model/tripo_convert_39079297-5fe4-439c-b021-f8ea45da3ef7.fbx",
                    rot=(0,0,0),   fit=('Z',24.0),  pivot='center'),
 "Backboard":  dict(src=D+"basketball+backboard+3d+model/tripo_convert_e7984f9c-bb6c-4f57-aeb7-8ca2d1649681.fbx",
                    rot=(0,0,0),   fit=('X',100.0), pivot='center'),   # UE mirrors Blender Y: red face already points UE +Y (players)
 "Boombox":    dict(src=D+"boombox+3d+model/tripo_convert_a624fbb3-9e36-4a66-b980-a1fd1d28fce1.fbx",
                    rot=(0,0,0),   fit=('X',60.0),  pivot='center'),
 "StunGun":    dict(src=D+"electric+3d+model/tripo_convert_cd83736a-ed12-48cf-a3a3-e67feba10ae5.fbx",
                    rot=(0,0,0),   fit=('Z',40.0),  pivot='center'),
 "Megaphone":  dict(src=D+"megaphone+3d+model/tripo_convert_75882902-6569-4ee1-95aa-983a42b52afb.fbx",
                    rot=(0,0,0),   fit=('Z',60.0),  pivot='center'),
 "StandMic":   dict(src=D+"standmic/standmic.fbx",
                    rot=(0,0,0),   fit=('Z',160.0), pivot='center'),
 "HoopRim":    dict(src=D+"low-poly ring 3d model.fbx",
                    rot=(0,0,180), fit=('X',50.0),  pivot='rimcenter'),  # X = rim diameter; 180 so bracket lands at UE -Y (board side)
}

def bbox(obj):
    pts = [obj.matrix_world @ v.co for v in obj.data.vertices]
    lo = mathutils.Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = mathutils.Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi, pts

def apply(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True); bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

ONLY = sys.argv[sys.argv.index('--')+2:]
for name, c in CFG.items():
    if ONLY and name not in ONLY: continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=c['src'])
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes: o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1: bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = "SM_" + name
    obj.parent = None
    apply(obj)

    # 1. rotate about the world origin
    obj.rotation_euler = tuple(math.radians(a) for a in c['rot'])
    apply(obj)

    # 2. pivot
    lo, hi, pts = bbox(obj)
    if c['pivot'] == 'center':
        pv = (lo + hi) / 2
    else:  # rimcenter: extreme-|x| verts lie on the rim's horizontal diameter -> give circle centre y and tube z
        w = max(abs(p.x) for p in pts)
        rim = [p for p in pts if abs(p.x) > w * 0.9]
        pv = mathutils.Vector((0.0, sum(p.y for p in rim)/len(rim), sum(p.z for p in rim)/len(rim)))
        print(f"  rim centre (src units) y={pv.y:.4f} z={pv.z:.4f}  rim tube verts={len(rim)}")
    obj.location = -pv
    apply(obj)

    # 3. uniform scale so the fit axis hits size_cm
    lo, hi, _ = bbox(obj)
    ax = 'XYZ'.index(c['fit'][0])
    dim = (hi - lo)[ax]
    s = (c['fit'][1] / 100.0) / dim
    obj.scale = (s, s, s)
    apply(obj)

    lo, hi, _ = bbox(obj)
    print(f"{name}: scale x{s:.3f}  final bbox (cm) X[{lo.x*100:.1f},{hi.x*100:.1f}] Y[{lo.y*100:.1f},{hi.y*100:.1f}] Z[{lo.z*100:.1f},{hi.z*100:.1f}]  verts={len(obj.data.vertices)}")

    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True)
    bpy.ops.export_scene.fbx(filepath=f"{OUT}/{name}.fbx", use_selection=True, object_types={'MESH'},
                             add_leaf_bones=False, bake_anim=False)
print("BAKE DONE")
