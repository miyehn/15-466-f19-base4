
BLENDER=/Applications/Blender.app/Contents/MacOS/Blender

all : \
	../dist/test_scene.pnct \
	../dist/test_scene.scene \

../dist/test_scene.pnct : sphere-roller.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- sphere-roller.blend:test_scene '$@'

../dist/test_scene.scene : sphere-roller.blend export-meshes.py
	$(BLENDER) --background --python export-scene.py -- sphere-roller.blend:test_scene '$@'

../dist/roll-parts.pnct : sphere-roller.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- sphere-roller.blend:Parts '$@'

../dist/roll-level-1.scene : sphere-roller.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- sphere-roller.blend:Level.1 '$@'

../dist/roll-level-2.scene : sphere-roller.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- sphere-roller.blend:Level.2 '$@'

../dist/roll-level-3.scene : sphere-roller.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- sphere-roller.blend:Level.3 '$@'

