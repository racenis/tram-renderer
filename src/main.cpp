#include <framework/core.h>
#include <framework/logging.h>
#include <framework/ui.h>
#include <framework/gui.h>
#include <framework/async.h>
#include <framework/event.h>
#include <framework/message.h>
#include <framework/system.h>
#include <framework/worldcell.h>
#include <framework/language.h>
#include <framework/file.h>
#include <framework/path.h>
#include <framework/stats.h>
#include <framework/script.h>
#include <framework/loader.h>
#include <framework/settings.h>
#include <components/trigger.h>
#include <audio/audio.h>
#include <audio/sound.h>
#include <render/render.h>
#include <render/material.h>
#include <render/api.h>
#include <render/scene.h>
#include <render/raytrace/raytrace.h>
#include <physics/physics.h>
#include <physics/api.h>
#include <platform/image.h>
#include <entities/player.h>
#include <entities/staticworldobject.h>
#include <entities/light.h>
#include <entities/crate.h>
#include <entities/marker.h>
#include <entities/trigger.h>
#include <entities/sound.h>
#include <entities/decoration.h>
#include <components/player.h>
#include <components/animation.h>
#include <components/controller.h>
#include <components/render.h>
#include <extensions/camera/camera.h>
#include <extensions/menu/menu.h>
#include <extensions/scripting/lua.h>
#include <extensions/kitchensink/kitchensink.h>
#include <extensions/kitchensink/entities.h>
#include <extensions/kitchensink/soundtable.h>
#include <extensions/kitchensink/imageassembly.h>

using namespace tram;
using namespace tram::UI;
using namespace tram::Render;
using namespace tram::Physics;
using namespace tram::Ext::Kitchensink;

struct AnimationSequence {
	name_t setup;
	name_t update;
	name_t teardown;
};

struct AnimationRender {
	int frames;
	float length;
};

static int current_frame = 0;
static bool paused = true;
static bool looping = false;
static bool previewing = true;
static bool create_assembly = false;

static AnimationSequence* current_sequence = nullptr;
static AnimationRender* current_render = nullptr;


void UpdateScene() {
	const float advance = current_render->length / (float)current_render->frames;
	
	Core::SetTime((float)current_frame * advance);

	Script::CallFunction(current_sequence->update, {});
}

void SaveFrame() {
	char* buffer = (char*)malloc(UI::GetScreenWidth() * UI::GetScreenHeight() * 3);

	std::string file_name = "frame";
	file_name += std::to_string(current_frame + 1);
	file_name += ".png";
	
	std::cout << "saved out: " << file_name << std::endl;
	
	Render::API::GetScreen(buffer, UI::GetScreenWidth(), UI::GetScreenHeight());
	Platform::SaveImageToDisk(file_name.c_str(), UI::GetScreenWidth(), UI::GetScreenHeight(), buffer);
	
	if (create_assembly) {
		Kitchensink::ImageAssembly::Add(Render::API::GetAssemblyLayers());
	}
	
	free(buffer);
}

void TryLooping() {
	if (current_frame >= current_render->frames) {
		previewing = true;
		
		if (looping) {
			current_frame = 0;
		} else {
			current_frame--;
		}
	}
	
	if (current_frame < 0) {
		previewing = true;
		
		if (looping) {
			current_frame = current_render->frames - 1;
		} else {
			current_frame++;
		}
	}
	
}

void main_loop();

int main(int argc, const char** argv) {
	Settings::Parse(argv, argc);
	
	Light::Register();
    Crate::Register();
    Sound::Register();
    Decoration::Register();
    Trigger::Register();
    StaticWorldObject::Register();
    Ext::Kitchensink::Button::Register();
	
	Core::SetPlatformTime(false);
	
	Core::Init();
	UI::Init();
	Render::Init();
	Physics::Init();
#ifdef __EMSCRIPTEN__
	Async::Init(0);
#else
	Async::Init();
#endif
	Audio::Init();
	GUI::Init();

	Ext::Menu::Init();
	Ext::Camera::Init();
	Ext::Kitchensink::Init();

	Ext::Scripting::Lua::Init();
	Script::Init();

	Material::LoadMaterialInfo("material");
	Language::Load("en");
	
	
	Script::SetFunction("AddSequence", {TYPE_NAME, TYPE_NAME, TYPE_NAME}, [](valuearray_t params) -> value_t {
		if (current_sequence) {
			if (current_frame) {
				Script::CallFunction(current_sequence->teardown, {});
			}
			
			delete current_sequence;
		}
		
		current_frame = 0;
		paused = true;
		
		current_sequence = new AnimationSequence {
			params[0],
			params[1],
			params[2]
		};
		
		Script::CallFunction(current_sequence->setup, {});
		
		return true;
	});
	
	Script::SetFunction("RenderSequence", {TYPE_INT32, TYPE_FLOAT32}, [](valuearray_t params) -> value_t {
		if (current_render) delete current_render;
		
		current_render = new AnimationRender {
			params[0],
			params[1]
		};
		
		return true;
	});
	
	Script::SetFunction("SetFrame", {TYPE_INT32}, [](valuearray_t params) -> value_t {
		current_frame = params[0]; return true;
	});
	
	Script::SetFunction("AdvanceFrame", {}, [](valuearray_t params) -> value_t {
		current_frame++; return true;
	});
	
	Script::SetFunction("ReverseFrame", {}, [](valuearray_t params) -> value_t {
		current_frame--; return true;
	});

	Script::SetFunction("SetLoop", {TYPE_BOOL}, [](valuearray_t params) -> value_t {
		looping = params[0]; return true;
	});
	
	Script::SetFunction("BeginRender", {}, [](valuearray_t params) -> value_t {
		previewing = false;
		paused = false;
		
		if (Render::API::GetContext() == Render::API::CONTEXT_SOFTWARE) {
			Render::API::SetInteractiveMode(false);
		}
		
		return true;
	});
	
	Script::SetFunction("CancelRender", {}, [](valuearray_t params) -> value_t {
		previewing = true;
		paused = true;
		
		return true;
	});
	
	Script::SetFunction("BeginPreview", {}, [](valuearray_t params) -> value_t {
		previewing = true;
		paused = false;
		
		return true;
	});
	
	Script::SetFunction("CancelPreview", {}, [](valuearray_t params) -> value_t {
		previewing = true;
		paused = true;
		
		return true;
	});
	
	Script::SetFunction("IsPaused", {}, [](valuearray_t params) -> value_t {
		return paused;
	});
	
	Script::SetFunction("GetCurrentFrame", {}, [](valuearray_t params) -> value_t {
		return current_frame;
	});
	
	Script::SetFunction("GetTotalFrames", {}, [](valuearray_t params) -> value_t {
		return current_render ? current_render->frames : 0;
	});
	
	Script::SetFunction("SetUseAssembly", {TYPE_BOOL}, [](valuearray_t params) -> value_t {
		create_assembly = params[0];
		Render::API::SetUseAssembly(create_assembly);
		return true;
	});
	
	Script::SetFunction("SetMaterialAssemblyIndex", {TYPE_NAME, TYPE_INT32}, [](valuearray_t params) -> value_t {
		auto material = Material::Find(params[0]);
		if (material->GetStatus() != Resource::LOADED) {
			material->Load();
		}
		Render::API::SetMaterialAssemblyIndex(material->GetMaterial(), params[1]);
		return true;
	});
	
	Script::LoadScript("init");
	
	#ifdef __EMSCRIPTEN__
		UI::SetWebMainLoop(main_loop);
	#else
		while (!UI::ShouldExit()) {
			main_loop();
		}

		Ext::Scripting::Lua::Uninit();
		
		Async::Yeet();
		Audio::Uninit();
		UI::Uninit();
	#endif
}

void main_loop() {
	Core::Update();
	UI::Update();
	Physics::Update();	

	// TODO: check if this logic works with opengl renderer (probably doesn't)
	
	// check if raytrace renderer is finished with current frame
	if (!previewing && Render::API::IsFinishedRendering()) {
		SaveFrame();
		
		// advance to next frame
		current_frame++;
		
		TryLooping();
		UpdateScene();
		
		// render wireframe
		Render::API::SetInteractiveMode(true);
		Render::Render();
		
		// start rendering the next frame
		if (!previewing) {
			Render::API::SetInteractiveMode(false);
		} else {
			if (create_assembly) {
				Kitchensink::ImageAssembly::Save("assembly.image");
				Kitchensink::ImageAssembly::Reset();
			}
		}
	}
	
	if (previewing) {
		if (!paused) {
			current_frame++;
		}
		
		TryLooping();
		UpdateScene();
	}
	
	GUI::Begin();
	Ext::Menu::Update();

	Event::Dispatch();
	Message::Dispatch();
	
	GUI::End();
	GUI::Update();
	
#ifdef __EMSCRIPTEN__
	Async::LoadResourcesFromDisk();
#endif
	Async::LoadResourcesFromMemory();
	Async::FinishResources();
	
	Loader::Update();
	
	AnimationComponent::Update();
	ControllerComponent::Update();
	
	Ext::Camera::Update();
	
	Render::Render();
	if (!previewing && Render::API::GetContext() != Render::API::CONTEXT_SOFTWARE) {
		UI::EndFrame();
		Render::Render();
	}
	UI::EndFrame();
	
	Stats::Collate();
}