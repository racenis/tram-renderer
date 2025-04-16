print("\n\nHello! This is the Tramway SDK rendering application.\n")
print("Here's some ideas for what you could do:")
print("- Press Space to preview the sequence.")
print("- Press Enter to render out the sequence.")
print("- Press Backspace to reset the sequence.")
print("- Press Left and Right to step through the sequence.")

-- Setting up the window.
tram.ui.SetWindowTitle("Teapot Renderer v1.0")
tram.ui.SetWindowSize(320, 240)

-- This makes the animation sequence repeat after it ends.
SetLoop(true)

-- This will allow us to retitle the window more easily during rendering.
function SetWindowTitle(title)
	local frame_text = GetCurrentFrame() .. "/" .. GetTotalFrames()
	tram.ui.SetWindowTitle(title .. " [" .. frame_text .. "]")
end



-- This function sets up the sequence.
function TeapotSetup()
	-- Setting up the global lighting.
	tram.render.SetSunColor(tram.math.vec3(0.0, 0.0, 0.0))
	tram.render.SetSunDirection(tram.math.DIRECTION_FORWARD)
	tram.render.SetAmbientColor(tram.math.vec3(0.1, 0.1, 0.1))
	tram.render.SetScreenClearColor(tram.render.COLOR_BLACK)

	-- Move the camera a bit away from the origin.
	tram.render.SetViewPosition(tram.math.DIRECTION_FORWARD * -1.2)

	-- Setting up a light so that you can see something.
	scene_light = tram.components.Light()
	scene_light:SetColor(tram.render.COLOR_WHITE)
	scene_light:SetLocation(tram.math.vec3(5.0, 5.0, 5.0))
	scene_light:Init()

	-- Adding a teapot to the scene.
	teapot = tram.components.Render()
	teapot:SetModel("teapot")
	teapot:Init()
end

-- This function tears down the sequence.
function TeapotTeardown()
	scene_light:Delete()
	teapot:Delete()
end

-- This function will get called before rendering every frame.
function TeapotUpdate()
	local teapot_rotation = tram.GetTickTime()
	local teapot_modifier = tram.math.vec3(0.0, teapot_rotation, 0.0)
	
	teapot:SetRotation(tram.math.quat(teapot_modifier))
	
	SetWindowTitle("Teapot Render")
end


-- Here we register in the sequence to have 40 frames and last ~6 seconds.
AddSequence("TeapotSetup", "TeapotUpdate", "TeapotTeardown")
RenderSequence(40, 2.0 * 3.14)

-- Goes back a frame in the sequence.
tram.ui.BindKeyboardKey(tram.ui.KEY_LEFT, function()
	ReverseFrame()
end)

-- Goes forward a frame in the sequence.
tram.ui.BindKeyboardKey(tram.ui.KEY_RIGHT, function()
	AdvanceFrame()
end)

-- Starts/pauses sequence preview.
tram.ui.BindKeyboardKey(tram.ui.KEY_SPACE, function()
	if IsPaused() then
		BeginPreview()
	else
		CancelPreview()
	end
end)

-- Starts/pauses rendering. 
tram.ui.BindKeyboardKey(tram.ui.KEY_ENTER, function()
	if IsPaused() then
		BeginRender()
	else
		CancelRender()
	end
end)

-- Resets the sequence.
tram.ui.BindKeyboardKey(tram.ui.KEY_BACKSPACE, function()
	if not IsPaused() then
		CancelPreview()
	end
	
	SetFrame(0)
end)
