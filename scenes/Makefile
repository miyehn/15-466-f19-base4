
BLENDER=/Applications/Blender.app/Contents/MacOS/Blender

all : \
	../dist/test_scene.pnct \
	../dist/test_scene.scene \

../dist/test_scene.pnct : sphere-roller.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- sphere-roller.blend:parts '$@'

../dist/test_scene.scene : sphere-roller.blend export-meshes.py
	$(BLENDER) --background --python export-scene.py -- sphere-roller.blend:test_scene '$@'
