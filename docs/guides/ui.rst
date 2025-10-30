User Interface
==============

Citrus Engine uses `ImGui <https://github.com/ocornut/imgui>`_ for creating
debug tools and editor interfaces with immediate mode rendering.

Overview
--------

ImGui provides:

* Immediate mode GUI (no retained state)
* Easy-to-use API
* Built-in widgets (buttons, sliders, text inputs, etc.)
* Docking and multi-viewport support
* Customizable styling

For in-game UI, see the UI system documentation in the API reference.

Getting Started
---------------

Basic Window
~~~~~~~~~~~~

.. code-block:: cpp

   import engine.ui;
   
   void RenderUI() {
       ImGui::Begin("My Window");
       ImGui::Text("Hello, Citrus Engine!");
       if (ImGui::Button("Click Me")) {
           // Button clicked
       }
       ImGui::End();
   }

Integration
~~~~~~~~~~~

.. code-block:: cpp

   // In game loop
   while (!window->ShouldClose()) {
       // Start new frame
       ImGui::NewFrame();
       
       // Render your UI
       RenderUI();
       
       // Render
       ImGui::Render();
       renderer->RenderImGui(ImGui::GetDrawData());
   }

Common Widgets
--------------

Text
~~~~

.. code-block:: cpp

   ImGui::Text("Static text");
   ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red text");
   ImGui::TextWrapped("Long text that wraps automatically...");

Buttons
~~~~~~~

.. code-block:: cpp

   if (ImGui::Button("Start Game")) {
       // Start game
   }
   
   if (ImGui::SmallButton("Small")) {
       // ...
   }
   
   // Directional buttons
   if (ImGui::ArrowButton("left", ImGuiDir_Left)) {
       // ...
   }

Input Fields
~~~~~~~~~~~~

.. code-block:: cpp

   static char name[128] = "";
   ImGui::InputText("Name", name, sizeof(name));
   
   static float value = 0.0f;
   ImGui::InputFloat("Value", &value);
   
   static int count = 0;
   ImGui::InputInt("Count", &count);

Sliders
~~~~~~~

.. code-block:: cpp

   static float volume = 1.0f;
   ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
   
   static int difficulty = 1;
   ImGui::SliderInt("Difficulty", &difficulty, 1, 10);
   
   static float color[3] = {1.0f, 1.0f, 1.0f};
   ImGui::ColorEdit3("Color", color);

Checkboxes and Radio
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   static bool enabled = true;
   ImGui::Checkbox("Enabled", &enabled);
   
   static int selection = 0;
   ImGui::RadioButton("Option 1", &selection, 0);
   ImGui::RadioButton("Option 2", &selection, 1);
   ImGui::RadioButton("Option 3", &selection, 2);

Combo Boxes
~~~~~~~~~~~

.. code-block:: cpp

   static int current_item = 0;
   const char* items[] = { "Item 1", "Item 2", "Item 3" };
   ImGui::Combo("Select", &current_item, items, IM_ARRAYSIZE(items));

Lists
~~~~~

.. code-block:: cpp

   static int selected = 0;
   const char* items[] = { "Apple", "Banana", "Cherry" };
   
   ImGui::ListBox("Fruits", &selected, items, IM_ARRAYSIZE(items), 4);

Layout
------

Grouping
~~~~~~~~

.. code-block:: cpp

   ImGui::BeginGroup();
   ImGui::Text("Group 1");
   ImGui::Button("Button 1");
   ImGui::Button("Button 2");
   ImGui::EndGroup();

Columns
~~~~~~~

.. code-block:: cpp

   ImGui::Columns(2);
   ImGui::Text("Column 1");
   ImGui::NextColumn();
   ImGui::Text("Column 2");
   ImGui::Columns(1);  // Reset

Spacing and Separators
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ImGui::Spacing();
   ImGui::Separator();
   ImGui::NewLine();
   
   ImGui::SameLine();  // Next item on same line

Windows and Panels
------------------

Window Flags
~~~~~~~~~~~~

.. code-block:: cpp

   ImGuiWindowFlags flags = 0;
   flags |= ImGuiWindowFlags_NoTitleBar;
   flags |= ImGuiWindowFlags_NoResize;
   flags |= ImGuiWindowFlags_NoMove;
   
   ImGui::Begin("Fixed Window", nullptr, flags);
   // ...
   ImGui::End();

Collapsing Headers
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   if (ImGui::CollapsingHeader("Settings")) {
       ImGui::Text("Setting 1");
       ImGui::Text("Setting 2");
   }

Tree Nodes
~~~~~~~~~~

.. code-block:: cpp

   if (ImGui::TreeNode("Scene")) {
       if (ImGui::TreeNode("Entities")) {
           ImGui::Text("Player");
           ImGui::Text("Enemy");
           ImGui::TreePop();
       }
       ImGui::TreePop();
   }

Tabs
~~~~

.. code-block:: cpp

   if (ImGui::BeginTabBar("MainTabs")) {
       if (ImGui::BeginTabItem("Scene")) {
           ImGui::Text("Scene content");
           ImGui::EndTabItem();
       }
       if (ImGui::BeginTabItem("Inspector")) {
           ImGui::Text("Inspector content");
           ImGui::EndTabItem();
       }
       ImGui::EndTabBar();
   }

Debug Tools
-----------

Performance Monitor
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ImGui::Begin("Performance");
   ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
   ImGui::PlotLines("Frame Times", frame_times, 100);
   ImGui::End();

Entity Inspector
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ImGui::Begin("Entity Inspector");
   
   if (selected_entity) {
       ImGui::Text("Entity: %s", selected_entity.GetName());
       
       if (auto* transform = selected_entity.GetComponent<Transform>()) {
           ImGui::DragFloat3("Position", &transform->position.x);
           ImGui::DragFloat4("Rotation", &transform->rotation.x);
           ImGui::DragFloat3("Scale", &transform->scale.x);
       }
   }
   
   ImGui::End();

Console
~~~~~~~

.. code-block:: cpp

   ImGui::Begin("Console");
   
   // Log output
   ImGui::BeginChild("ScrollingRegion", ImVec2(0, -30));
   for (auto& log : console_logs) {
       ImGui::TextUnformatted(log.c_str());
   }
   ImGui::EndChild();
   
   // Input
   static char input[256] = "";
   if (ImGui::InputText("Command", input, sizeof(input), 
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
       // Execute command
       ExecuteCommand(input);
       input[0] = '\0';
   }
   
   ImGui::End();

Styling
-------

Colors
~~~~~~

.. code-block:: cpp

   ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
   ImGui::Button("Red Button");
   ImGui::PopStyleColor();

Custom Style
~~~~~~~~~~~~

.. code-block:: cpp

   ImGuiStyle& style = ImGui::GetStyle();
   style.WindowRounding = 5.0f;
   style.FrameRounding = 3.0f;
   style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);

Fonts
~~~~~

.. code-block:: cpp

   ImGuiIO& io = ImGui::GetIO();
   ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto.ttf", 16.0f);
   
   // Use font
   ImGui::PushFont(font);
   ImGui::Text("Custom font");
   ImGui::PopFont();

Best Practices
--------------

1. **Static variables**: Use static variables for widget state
2. **Unique IDs**: Use ``##`` for invisible IDs when needed
3. **Begin/End pairs**: Always match Begin/End calls
4. **Performance**: Minimize widget creation in hot loops
5. **Input handling**: Check ``WantCaptureMouse`` and ``WantCaptureKeyboard``

Input Handling
--------------

Checking Input Capture
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ImGuiIO& io = ImGui::GetIO();
   
   // Don't process game input if ImGui wants the mouse
   if (!io.WantCaptureMouse) {
       // Handle game mouse input
   }
   
   // Don't process game input if ImGui wants the keyboard
   if (!io.WantCaptureKeyboard) {
       // Handle game keyboard input
   }

Examples
--------

Simple Editor
~~~~~~~~~~~~~

.. code-block:: cpp

   void RenderEditor() {
       // Main menu bar
       if (ImGui::BeginMainMenuBar()) {
           if (ImGui::BeginMenu("File")) {
               if (ImGui::MenuItem("New")) { /* ... */ }
               if (ImGui::MenuItem("Open")) { /* ... */ }
               if (ImGui::MenuItem("Save")) { /* ... */ }
               ImGui::EndMenu();
           }
           ImGui::EndMainMenuBar();
       }
       
       // Scene hierarchy
       ImGui::Begin("Scene");
       for (auto& entity : scene.GetEntities()) {
           if (ImGui::Selectable(entity.GetName().c_str())) {
               selected_entity = entity;
           }
       }
       ImGui::End();
       
       // Properties panel
       ImGui::Begin("Properties");
       if (selected_entity) {
           RenderEntityProperties(selected_entity);
       }
       ImGui::End();
   }

See Also
--------

* `ImGui Documentation <https://github.com/ocornut/imgui/wiki>`_
* `ImGui Demo <https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp>`_
