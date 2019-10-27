
BLENDER=/Applications/Blender.app/Contents/MacOS/Blender

all : \
	../dist/test_scene.pnct \
	../dist/test_scene.scene \
	solidarity.pnct \

../dist/test_scene.pnct : scene-editor.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- scene-editor.blend:parts '$@'

../dist/test_scene.scene : scene-editor.blend export-meshes.py
	$(BLENDER) --background --python export-scene.py -- scene-editor.blend:test_scene '$@'

solidarity.pnct: scene-editor.blend export-meshes.py
	$(BLENDER) --background --python export-scene.py -- scene-editor.blend:solidarity '$@'
