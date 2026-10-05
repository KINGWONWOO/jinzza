# Headless Blender script: builds the Room Settings gear icon and exports SM_Gear.fbx.
#   "C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" -b -P docs/models/make_gear.py
# Gear stands upright (face toward +/-Y), centred on the origin, 1 m across.
import bpy, bmesh, math, os

bpy.ops.wm.read_factory_settings(use_empty=True)

TEETH, R_ROOT, R_TIP, R_HOLE, THICK = 12, 0.38, 0.5, 0.16, 0.16
outer, inner = [], []
step = 2 * math.pi / TEETH
for t in range(TEETH):
    a = t * step
    # root -> tip -> tip -> root, tips slightly narrower than roots (trapezoid teeth)
    for frac, r in ((0.0, R_ROOT), (0.18, R_TIP), (0.42, R_TIP), (0.6, R_ROOT)):
        outer.append((a + frac * step, r))
    # (0.6..1.0 of each step is the gap at root radius, closed by the next tooth's first point)
n = len(outer)
for i in range(n):
    inner.append((outer[i][0], R_HOLE))

bm = bmesh.new()
def ring(pts, y):
    return [bm.verts.new((r * math.cos(a), y, r * math.sin(a))) for a, r in pts]
of, ob = ring(outer, THICK / 2), ring(outer, -THICK / 2)
inf, inb = ring(inner, THICK / 2), ring(inner, -THICK / 2)
for i in range(n):
    j = (i + 1) % n
    bm.faces.new((of[i], of[j], inf[j], inf[i]))      # front face
    bm.faces.new((ob[j], ob[i], inb[i], inb[j]))      # back face
    bm.faces.new((ob[i], ob[j], of[j], of[i]))        # outer wall (teeth)
    bm.faces.new((inf[i], inf[j], inb[j], inb[i]))    # hole wall
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

me = bpy.data.meshes.new("SM_Gear")
bm.to_mesh(me)
bm.free()
ob_ = bpy.data.objects.new("SM_Gear", me)
bpy.context.collection.objects.link(ob_)
bpy.context.view_layer.objects.active = ob_
ob_.select_set(True)

bev = ob_.modifiers.new("Bevel", "BEVEL")
bev.width, bev.segments, bev.limit_method = 0.012, 2, 'ANGLE'
bpy.ops.object.modifier_apply(modifier="Bevel")
bpy.ops.object.shade_auto_smooth(angle=math.radians(35))

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "SM_Gear.fbx")
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, apply_unit_scale=True,
                         mesh_smooth_type='FACE', add_leaf_bones=False)
print("EXPORTED", out, len(me.vertices))
