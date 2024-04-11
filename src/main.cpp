#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path, bool gamma);

unsigned int loadCubemap(vector<std::string> faces);

void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 housePosition = glm::vec3(0.0f);
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Pravimo prozor
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Haunted house", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // Kazemo openGl da nacrta prozor
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader shader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader floorShader("resources/shaders/parallax.vs", "resources/shaders/parallax.fs");
    // load models
    // -----------
    Model ourModel("resources/objects/kuca/Manor.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model spiderModel("resources/objects/pauk/Spider.obj");
    spiderModel.SetShaderTextureNamePrefix("material.");

    Model pumpkinModel("resources/objects/bundeva/pumpkin.obj");
    pumpkinModel.SetShaderTextureNamePrefix("material.");


    float planeVertices[] = {
            5.0f, 0.0f, 5.0f, 20.0f, 0.0f,
            -5.0f,0.0f, 5.0f, 0.0f, 0.0f,
            -5.0f, 0.0f, -5.0f, 0.0f, 20.0f,

            5.0f, 0.0f, 5.0f, 20.0f, 0.0f,
            -5.0f, 0.0f, -5.0,0.0f, 20.0f,
            5.0f, 0.0f, -5.0f, 20.0f, 20.0f
    };

    float transparentVertices[] = {
            0.0f,  0.5f,  0.0f, 1.0f, 1.0f,
            0.0f, -0.5f,  0.0f, 1.0f,0.0f,
            1.0f, -0.5f,  0.0f, 0.0f,0.0f,

            0.0f,  0.5f,  0.0f,  1.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  0.0f, 1.0f
    };

    float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    //plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str(), true);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int diffuseMap = loadTexture("resources/textures/grass/diffuse.png", true);
    unsigned int normalMap = loadTexture("resources/textures/grass/normal.png", true);
    unsigned int heightMap = loadTexture("resources/textures/grass/height_map.jpg", true);

    vector<glm::vec3> vegetation
            {
                    glm::vec3(-11.5f, 0.5f, -10.48f),
                    glm::vec3( 11.5f, 0.5f, 10.51f),
                    glm::vec3( 12.0f, 0.5f, 10.7f),
                    glm::vec3(-13.3f, 0.5f, -12.3f),
                    glm::vec3 (13.5f, 0.5f, -10.6f),
                    glm::vec3(-12.5f, 0.5f, -10.4f),
                    glm::vec3( 12.5f, 0.5f, 10.5f),
                    glm::vec3( 11.0f, 0.5f, 10.7f),
                    glm::vec3(-15.3f, 0.5f, -12.3f),
                    glm::vec3 (15.5f, 0.5f, -10.6f),
                    glm::vec3(-11.5f, 0.5f, -11.4f),
                    glm::vec3( 11.5f, 0.5f, 12.5f),
                    glm::vec3( 12.0f, 0.5f, 13.7f),
                    glm::vec3(-13.3f, 0.5f, -12.3f),
                    glm::vec3 (13.5f, 0.5f, -11.6f),
                    glm::vec3(-12.5f, 0.5f, -12.4f),
                    glm::vec3( 12.5f, 0.5f, 15.5f),
                    glm::vec3( 11.0f, 0.5f, 14.7f),
                    glm::vec3(-15.3f, 0.5f, -13.3f),
                    glm::vec3 (15.5f, 0.5f, -12.6f),
                    glm::vec3(-12.5f, 0.5f, -13.48f),
                    glm::vec3( 3.5f, 0.5f, 2.51f),
                    glm::vec3( 4.0f, 0.5f, 1.7f),
                    glm::vec3(-3.3f, 0.5f, -2.3f),
                    glm::vec3 (5.5f, 0.5f, -3.6f),
                    glm::vec3(-6.5f, 0.5f, -1.48f),
                    glm::vec3( 1.5f, 0.5f, 2.51f),
                    glm::vec3( 1.0f, 0.5f, 3.7f),
                    glm::vec3(-2.3f, 0.5f, -2.3f),
                    glm::vec3 (3.5f, 0.5f, -3.6f),
                    glm::vec3(-1.5f, 0.5f, -0.48f),
                    glm::vec3( 1.5f, 0.5f, 0.51f),
                    glm::vec3( 2.0f, 0.5f, 0.7f),
                    glm::vec3(-3.3f, 0.5f, -2.3f),
                    glm::vec3 (3.5f, 0.5f, -0.6f),
                    glm::vec3(-2.5f, 0.5f, -0.48f),
                    glm::vec3( 2.5f, 0.5f, 0.51f),
                    glm::vec3( 1.0f, 0.5f, 0.7f),
                    glm::vec3(-5.3f, 0.5f, -2.3f),
                    glm::vec3 (5.5f, 0.5f, -0.6f),
                    glm::vec3(2.5f, 0.5f, -1.9f),
                    glm::vec3( 5.5f, 0.5f, 2.51f),
                    glm::vec3( 2.0f, 0.5f, 3.9f),
                    glm::vec3(3.3f, 0.5f, -4.3f),
                    glm::vec3 (3.5f, 0.5f, -1.9f),
                    glm::vec3(2.5f, 0.5f, -2.78f),
                    glm::vec3( 1.5f, 0.5f, 5.51f),
                    glm::vec3( 1.0f, 0.5f, 4.7f),
                    glm::vec3(5.3f, 0.5f, 3.3f),
                    glm::vec3 (5.5f, 0.5f, 2.6f),
                    glm::vec3(2.5f, 0.5f, 3.48f),
                    glm::vec3( 3.5f, 0.5f, 2.51f),
                    glm::vec3( 4.0f, 0.5f, -1.7f),
                    glm::vec3(-3.3f, 0.5f, 2.3f),
                    glm::vec3 (5.5f, 0.5f, 3.6f),
                    glm::vec3(-6.5f, 0.5f, 1.48f),
                    glm::vec3( 1.5f, 0.5f, 2.51f),
                    glm::vec3( 1.0f, 0.5f, 3.7f),
                    glm::vec3(-2.3f, 0.5f, -2.3f),
                    glm::vec3 (3.5f, 0.5f, -3.6f)
            };


    shader.use();
    shader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    floorShader.use();
    floorShader.setInt("diffuseMap", 0);
    floorShader.setInt("normalMap", 1);
    floorShader.setInt("depthMap", 2);

    glm::vec3 lightPos(1.5f, 1.0f, 1.3f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
       // glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms

        ourShader.use();

        ourShader.setVec3("dirLight.direction", 0.0f, -5.0f, 0.0f);
        ourShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        ourShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        ourShader.setVec3("pointLight.position", 5.0f , 5.0f, 5.0f );
        ourShader.setVec3("pointLight.ambient", 1.05f, 1.05f, 1.05f);
        ourShader.setVec3("pointLight.diffuse", 1.4f, 1.4f, 1.4f);
        ourShader.setVec3("pointLight.specular", 1.5f, 1.5f, 1.5f);
        ourShader.setFloat("pointLight.constant", 1.0f);
        ourShader.setFloat("pointLight.linear", 0.09f);
        ourShader.setFloat("pointLight.quadratic", 0.032f);

        ourShader.setVec3("lampPointLight.position", 4.0f, 4.0f, 0.0f);
        ourShader.setVec3("lampPointLight.ambient", 0.05f, 0.05f, 0.05f);
        ourShader.setVec3("lampPointLight.diffuse", 0.4f, 0.4f, 0.4f);
        ourShader.setVec3("lampPointLight.specular", 0.5f, 0.5f, 0.5f);
        ourShader.setFloat("lampPointLight.constant", 1.0f);
        ourShader.setFloat("lampPointLight.linear", 0.09f);
        ourShader.setFloat("lampPointLight.quadratic", 0.032f);

        ourShader.setVec3("spotLight.direction", 0.0f, -1.0f, 0.0f);
        ourShader.setVec3("spotLight.position", 4.0f, 4.0f, 4.0f);
        ourShader.setVec3("spotLight.ambient", 0.05f, 0.05f, 0.05f);
        ourShader.setVec3("spotLight.diffuse", 0.4f, 0.4f, 0.4f);
        ourShader.setVec3("spotLight.specular", 0.5f, 0.5f, 0.5f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(20.0f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(35.0f)));

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);



        // render model house
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f,0.0f,0.0f));
        model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        //render model spider
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 2.0f));
        //model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setMat4("model", model);
        spiderModel.Draw(ourShader);

        //render model pumpkin
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 10.0f, 20.0f));
        model = glm::scale(model, glm::vec3(0.150f, 0.150f, 0.150f));
        ourShader.setMat4("model", model);
        pumpkinModel.Draw(ourShader);

        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        floorShader.setMat4("model", model);
        floorShader.setVec3("viewPos", programState->camera.Position);
        floorShader.setFloat("heightScale", 0.1f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMap);
        renderQuad();



        shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {

            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);

            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }
        glDisable(GL_CULL_FACE);


        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
         //glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);



        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &transparentVBO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path, bool gamma)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum dataFormat;
        GLenum internalFormat;
        if (nrComponents == 1)
            internalFormat = dataFormat = GL_RED;
        else if (nrComponents == 3) {
            internalFormat = gamma ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4){
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, dataFormat == GL_RGBA ? GL_CLAMP_TO_EDGE :GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, dataFormat == GL_RGBA ? GL_CLAMP_TO_EDGE :GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-100.0f,  100.0f, 0.0f);
        glm::vec3 pos2(-100.0f, -100.0f, 0.0f);
        glm::vec3 pos3( 100.0f, -100.0f, 0.0f);
        glm::vec3 pos4( 100.0f,  100.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
