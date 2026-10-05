# Headless Blender script: builds the Start Match "play button" icon and exports SM_PlayButton.fbx.
#   "C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" -b -P docs/models/make_play_button.py
# Sticker-style round button (not the YouTube red rectangle): a 130 cm disc facing -X (the kiosk
# front) with a white rim ring and a chunky rounded play triangle in ink. UE mirrors Blender Y; the
# triangle tip is modelled at Blender +Y and BP_Kiosk_StartMatch's Model has roll 180 so it points
# right for a player standing in front.
# Material slots: "Plate" (face), "Rim" (white ring), "Icon" (triangle).
import bpy, bmesh, math, os

bpy.ops.wm.read_factory_settings(use_empty=True)
R, THICK = 0.65, 0.22

def finish(obj, mat_name, bevel=None):
    obj.data.materials.append(bpy.data.materials.new(mat_name))
    if bevel:
        bpy.context.view_layer.objects.active = obj
        b = obj.modifiers.new("Bevel", "BEVEL")
        b.width, b.segments = bevel
        bpy.ops.object.modifier_apply(modifier="Bevel")

# Face disc, axis along X.
bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=R * 0.9, depth=THICK, rotation=(0, math.radians(90), 0))
plate = bpy.context.active_object; plate.name = "Plate"
finish(plate, "Plate", (0.05, 4))

# White rim ring around the face, a little proud of it on both sides.
bpy.ops.mesh.primitive_torus_add(major_radius=R * 0.9, minor_radius=R * 0.1, major_segments=64, minor_segments=12,
                                 rotation=(0, math.radians(90), 0))
rim = bpy.context.active_object; rim.name = "Rim"
rim.scale = (1.0, 1.0, 1.0)
finish(rim, "Rim")

# Play triangle in the YZ plane, extruded toward -X, rounded by a strong bevel.
bm = bmesh.new()
h = 0.27
vs = [bm.verts.new(p) for p in ((0.0, 0.25, 0.0), (0.0, -0.15, h), (0.0, -0.15, -h))]
f = bm.faces.new(vs)
ext = bmesh.ops.extrude_face_region(bm, geom=[f])
for v in [e for e in ext["geom"] if isinstance(e, bmesh.types.BMVert)]:
    v.co.x -= 0.09
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
me = bpy.data.meshes.new("Icon"); bm.to_mesh(me); bm.free()
icon = bpy.data.objects.new("Icon", me); bpy.context.collection.objects.link(icon)
icon.location.x = -THICK / 2 + 0.02
finish(icon, "Icon", (0.035, 3))

bpy.ops.object.select_all(action='DESELECT')
for o in (plate, rim, icon): o.select_set(True)
bpy.context.view_layer.objects.active = plate
bpy.ops.object.join()
obj = bpy.context.active_object; obj.name = "SM_PlayButton"
bpy.ops.object.shade_auto_smooth(angle=math.radians(35))

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "SM_PlayButton.fbx")
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, apply_unit_scale=True, add_leaf_bones=False)
print("EXPORTED", out, len(obj.data.vertices), [m.name for m in obj.data.materials])
