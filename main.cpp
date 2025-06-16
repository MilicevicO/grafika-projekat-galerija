 //Autor: Ognjen Milicevic
//Opis: Projektni zadatak "Galerija"

#define _CRT_SECURE_NO_WARNINGS


#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <map>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "model.hpp"
#include "camera.h"

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.8f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -180.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isDoorOpen = false;


#include <ft2build.h>
#include FT_FREETYPE_H

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, unsigned int shaderId, unsigned int VAO, unsigned int VBO);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

std::map<char, Character> Characters;
int main(void)
{

    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    const char wTitle[] = "[Galerija]";
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, wTitle, NULL, NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }



    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //////////////////////////////////////////////////////////////////

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "fonts/arial.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    unsigned int textShader = createShader("text.vert", "text.frag");


    glUseProgram(textShader);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"),
        1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(0);
    //init buffers for text rendering
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /////////////////////////////////////////////////////////////////////


    unsigned int basicColorShader = createShader("basic.vert", "universalRect.frag");


    Model galleryModel("res/gallery/v1.obj");

    glm::vec3 doorHinge(1.0f, -0.4f, 0.3f);

    const double targetFrameTime = 1.0 / 60.0;

    glClearColor(0.529f, 0.808f, 0.922f, 1.0f);


    glm::vec4 galleryColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
    glm::vec4 doorColor = glm::vec4(0.588f, 0.294f, 0.0f, 0.5f);

    glm::vec4 painting1Color = glm::vec4(1.0, 0.0, 0.0, 1.0);
    glm::vec4 painting2Color = glm::vec4(0.0, 1.0, 0.0, 1.0);
    glm::vec4 painting3Color = glm::vec4(0.0, 0.0, 1.0, 1.0);


    while (!glfwWindowShouldClose(window))
    {

        auto startTime = std::chrono::high_resolution_clock::now();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);

        glUseProgram(basicColorShader);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(basicColorShader, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(basicColorShader, "view"), 1, GL_FALSE, &view[0][0]);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(basicColorShader, "model"), 1, GL_FALSE, &model[0][0]);

        glUniform4fv(glGetUniformLocation(basicColorShader, "uCol"), 1, &galleryColor[0]);

        galleryModel.DrawByName("GalleryBody", basicColorShader);

        glDisable(GL_CULL_FACE);

        galleryModel.DrawByName("FrontWall1", basicColorShader);
        galleryModel.DrawByName("FrontWall2", basicColorShader);
        galleryModel.DrawByName("FrontWall3", basicColorShader);


        glUniform4fv(glGetUniformLocation(basicColorShader, "uCol"), 1, &painting1Color[0]);

        galleryModel.DrawByName("Painting1", basicColorShader);

        glUniform4fv(glGetUniformLocation(basicColorShader, "uCol"), 1, &painting2Color[0]);

        galleryModel.DrawByName("Painting2", basicColorShader);

        glUniform4fv(glGetUniformLocation(basicColorShader, "uCol"), 1, &painting3Color[0]);

        galleryModel.DrawByName("Painting3", basicColorShader);

        glm::mat4 doorModel = model;

        if (isDoorOpen) 
        {

            // Open door transformations
            glm::mat4 doorModelTemp = glm::mat4(1.0f);
            doorModelTemp = glm::translate(doorModelTemp, doorHinge);
            doorModelTemp = glm::rotate(doorModelTemp, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            doorModelTemp = glm::translate(doorModelTemp, -doorHinge);

            doorModel = doorModelTemp;
        }

        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniformMatrix4fv(glGetUniformLocation(basicColorShader, "model"), 1, GL_FALSE, &doorModel[0][0]);
        glUniform4fv(glGetUniformLocation(basicColorShader, "uCol"), 1, &doorColor[0]);
        galleryModel.DrawByName("Door", basicColorShader);

        glDisable(GL_BLEND);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        RenderText("Ognjen Milicevic RA149/2020", 0.0f, 10.0f, 0.7f, glm::vec3(0.0, 0.0, 0.0), textShader, VAO, VBO);

        glfwSwapBuffers(window);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        double frameDuration = elapsedTime.count();
        double sleepTime = targetFrameTime - frameDuration;

        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
    }
    glDeleteProgram(basicColorShader);
    glDeleteProgram(textShader);

    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);
    
    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, unsigned int shaderId, unsigned int VAO, unsigned int VBO) {
    // Save previous OpenGL states
    GLboolean blendingEnabled;
    glGetBooleanv(GL_BLEND, &blendingEnabled);
    GLint currentBlendSrc, currentBlendDst;
    glGetIntegerv(GL_BLEND_SRC, &currentBlendSrc);
    glGetIntegerv(GL_BLEND_DST, &currentBlendDst);
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

    // Enable blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Activate corresponding render state	
    glUseProgram(shaderId);
    glUniform3f(glGetUniformLocation(shaderId, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursors for next glyph
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (1/64th units)
    }

    // Restore previous OpenGL states
    if (!blendingEnabled) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(currentBlendSrc, currentBlendDst);
    glUseProgram(currentProgram);
    glBindVertexArray(currentVAO);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//test123
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(ROTATE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(ROTATE_RIGHT, deltaTime);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{ 
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        isDoorOpen = !isDoorOpen;  // Toggle the upper door state
    }
}