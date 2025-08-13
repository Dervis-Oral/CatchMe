#define FREEGLUT_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <ft2build.h>
#include "stb_image.h"
#include FT_FREETYPE_H
#include <functional>

using namespace std;

class Window {
public:
    GLFWwindow* window;
    int w, h;

    Window(int width, int height, const char* title) : w(width), h(height) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(w, h, title, NULL, NULL); glfwMakeContextCurrent(window); glewInit();

        glViewport(0, 0, w, h); glEnable(GL_DEPTH_TEST); glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    }

    bool shouldClose() { return glfwWindowShouldClose(window); }
    void swap() { glfwSwapBuffers(window); }
    void poll() { glfwPollEvents(); }
    ~Window() { glfwDestroyWindow(window); glfwTerminate(); }
};

class Shader {
public:
    GLuint ID;

    Shader(const char* vertex, const char* fragment) {
        GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vShader, 1, &vertex, NULL); glCompileShader(vShader);

        GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fShader, 1, &fragment, NULL); glCompileShader(fShader);

        ID = glCreateProgram();
        glAttachShader(ID, vShader); glAttachShader(ID, fShader); glLinkProgram(ID);

        glDeleteShader(vShader); glDeleteShader(fShader);
    }

    void use() { glUseProgram(ID); }
    void setMat4(string name, const glm::mat4& mat) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void setVec4(string name, const glm::vec4& value) {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }
    ~Shader() { glDeleteProgram(ID); }
};

class Camera {
public:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    glm::mat4 view;
    glm::mat4 proj;
    float w, h;
    float yaw, pitch;
    double lastX, lastY;
    bool firstMouse;
    glm::vec3 target;
    float dist;
    float mouseSensitivity; 

    Camera(float width, float height) : pos(0.0f, 5.0f, -5.0f), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f),
        w(width), h(height), yaw(-90.0f), pitch(-15.0f), lastX(width / 2.0), lastY(height / 2.0), firstMouse(true),
        target(0.0f, 0.5f, 0.0f), dist(7.0f), mouseSensitivity(0.5f) { 
        proj = glm::perspective(glm::radians(45.0f), w / h, 0.1f, 100.0f);
        updateView(0.016f);
    }

    void updateView(float dt) {
        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(dir);

        glm::vec3 desiredPos = target - (front * dist);
        desiredPos.y = target.y + 3.0f; 

        // Wall conflict
        const float wallRadius = 10.0f; 
        const float wallThickness = 0.5f; 
        float cameraDist = glm::length(glm::vec2(desiredPos.x, desiredPos.z));
        if (cameraDist > wallRadius - wallThickness) {
            // Kamerayı duvarın dışına it
            float angle = atan2(desiredPos.z, desiredPos.x);
            desiredPos.x = (wallRadius - wallThickness - 0.5f) * cos(angle); // 0.5f kamera yarıçapı
            desiredPos.z = (wallRadius - wallThickness - 0.5f) * sin(angle);
            // dist değerini değiştirmiyoruz, sabit kalıyor
        }

        pos = desiredPos;
        view = glm::lookAt(pos, target, up);
    }

    void mouseInput(Window& win, double xpos, double ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoff = static_cast<float>(xpos - lastX);
        float yoff = static_cast<float>(lastY - ypos);
        lastX = xpos;
        lastY = ypos;

        // Apply mouseSensitivity to scale the offsets
        xoff *= 0.2f * mouseSensitivity;
        yoff *= 0.2f * mouseSensitivity;

        yaw += xoff;
        pitch += yoff;

        if (pitch > 45.0f) pitch = 45.0f;
        if (pitch < -45.0f) pitch = -45.0f;

        updateView(0.016f);
    }

    void setTarget(const glm::vec3& newTarget) {
        target = newTarget + glm::vec3(0.0f, 0.5f, 0.0f);
    }

    glm::vec3 getRight() const {
        return glm::normalize(glm::cross(up, front));
    }
};

class Mesh {
public:
    GLuint VAO, VBO, EBO;
    vector<GLfloat> verts;
    vector<GLuint> inds;

    Mesh(const vector<GLfloat>& vertices, const vector<GLuint>& indices) : verts(vertices), inds(indices) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(GLuint), inds.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, inds.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

class TextureMesh {
public:
    GLuint VAO, VBO, EBO;
    vector<GLfloat> verts;
    vector<GLuint> inds;

    // Varsayılan constructor (UI için, 2D vertex'ler)
    TextureMesh() {
        // 2D dörtgen için vertex'ler: pozisyon (x, y) ve doku koordinatları (u, v)
        verts = {
            -125.0f, -125.0f,  0.0f, 0.0f,  // Sol alt (150x150 piksel)
             125.0f, -125.0f,  1.0f, 0.0f,  // Sağ alt
             125.0f,  125.0f,  1.0f, 1.0f,  // Sağ üst
            -125.0f,  125.0f,  0.0f, 1.0f   // Sol üst
        };

        inds = {
            0, 1, 2,
            2, 3, 0
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(GLuint), inds.data(), GL_STATIC_DRAW);

        // Pozisyon (2) + Doku koordinatları (2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Parametreli constructor (zemin için, 3D vertex'ler)
    TextureMesh(const vector<GLfloat>& vertices, const vector<GLuint>& indices) : verts(vertices), inds(indices) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(GLuint), inds.data(), GL_STATIC_DRAW);

        // Pozisyon (3) + Doku koordinatları (2)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, inds.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~TextureMesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

class Texture {
public:
    GLuint ID;
    int width, height;

    Texture(const char* path) {
        glGenTextures(1, &ID);
        bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path, &width, &height, nullptr, 4);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            std::cerr << "Failed to load texture: " << path << std::endl;
            unsigned char white[] = { 255, 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
            width = height = 1;
        }
        stbi_image_free(data);
    }

    void bind(GLenum textureUnit = GL_TEXTURE0) const {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, ID);
    }

    ~Texture() {
        glDeleteTextures(1, &ID);
    }
};

class Renderer {
public:
    void draw(const Mesh& mesh, Shader& shader, const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model, const glm::vec4& color) {
        shader.use();
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", proj);
        shader.setVec4("color", color);

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.inds.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Tek bir drawTexture fonksiyonu: hem 3D hem 2D için kullanılabilir
    void drawTexture(const TextureMesh& mesh, Shader& shader, Texture& texture,
        const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
        const glm::vec4& colorTint, bool is2D = false) {
        shader.use();
        shader.setMat4("model", model);
        if (!is2D) {
            shader.setMat4("view", view);
        }
        shader.setMat4("projection", proj);
        shader.setVec4("colorTint", colorTint);

        texture.bind(GL_TEXTURE0); // Artık parametre kabul ediyor
        glUniform1i(glGetUniformLocation(shader.ID, "texture1"), 0);

        if (is2D) {
            glDisable(GL_DEPTH_TEST);
        }
        mesh.draw();
        if (is2D) {
            glEnable(GL_DEPTH_TEST);
        }
    }
};

class TextRenderer {
public:
    struct Char {
        GLuint texID;
        glm::ivec2 size;
        glm::ivec2 bearing;
        GLuint advance;
    };

    map<char, Char> chars;
    Shader* shader;
    GLuint VAO, VBO;

    TextRenderer(Shader* s) : shader(s) {
        FT_Library ft;
        FT_Init_FreeType(&ft);

        FT_Face face;
        FT_New_Face(ft, "C:\\Windows\\Fonts\\arial.ttf", 0, &face);
        FT_Set_Pixel_Sizes(face, 0, 48);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (GLubyte c = 0; c < 128; c++) {
            FT_Load_Char(face, c, FT_LOAD_RENDER);

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            Char ch = { tex, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                        static_cast<GLuint>(face->glyph->advance.x) };
            chars[c] = ch;
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
        glBindVertexArray(0);
    }

    void draw(string text, float x, float y, float scale, glm::vec3 color) {
        shader->use();
        glUniform3f(glGetUniformLocation(shader->ID, "textColor"), color.x, color.y, color.z);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        for (char c : text) {
            Char ch = chars[c];

            GLfloat xpos = x + ch.bearing.x * scale;
            GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

            GLfloat w = ch.size.x * scale;
            GLfloat h = ch.size.y * scale;

            GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };

            glBindTexture(GL_TEXTURE_2D, ch.texID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            x += (ch.advance >> 6) * scale;
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~TextRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        for (auto& pair : chars) glDeleteTextures(1, &pair.second.texID);
    }
};

class Player {
public:
    glm::vec3 pos, rollDir, lastValidRollDir;
    float speed = 3.0f, rollTime, jumpVel, dashTime, dashCool, superJumpCool, lastMoveTime;
    bool rolling, jumping, dashing, waveTriggered, superJumpUsed, isMoving, enableAbilities;
    const float rollDur = 1.0f, gravity = 9.8f, jumpPower = 4.0f, superJumpPower = 10.0f, dashDur = 0.2, dashSpeed = 12.0f, maxDashCool = 8.0f, maxSuperJumpCool = 15.0f, stopRollDur = 0.2f;

    Player(glm::vec3 startPos = glm::vec3(0.0f), bool enableAbilities = true) : pos(startPos), rolling(false), rollTime(0.0f),
        jumping(false), jumpVel(0.0f), dashing(false), dashTime(0.0f), dashCool(0.0f), superJumpCool(0.0f),
        waveTriggered(false), superJumpUsed(false), isMoving(false), lastMoveTime(0.0f), enableAbilities(enableAbilities) {
    }

    void update(Window& win, Camera& cam, float dt) {
        glm::vec3 dir(0.0f);
        bool movingNow = false;

        if (glfwGetKey(win.window, GLFW_KEY_W) == GLFW_PRESS) { dir += cam.front; movingNow = true; }
        if (glfwGetKey(win.window, GLFW_KEY_S) == GLFW_PRESS) { dir -= cam.front; movingNow = true; }
        if (glfwGetKey(win.window, GLFW_KEY_A) == GLFW_PRESS) { dir += cam.getRight(); movingNow = true; }
        if (glfwGetKey(win.window, GLFW_KEY_D) == GLFW_PRESS) { dir -= cam.getRight(); movingNow = true; }

        if (movingNow != isMoving) {
            isMoving = movingNow;
            if (isMoving) {
                rolling = true;
                rollTime = 0.0f;
                dir.y = 0.0f;
                if (glm::length(dir) > 0.001f) {
                    rollDir = glm::normalize(dir);
                }
            }
            lastMoveTime = static_cast<float>(glfwGetTime());
        }

        if (isMoving && (!dashing || !enableAbilities)) {
            dir.y = 0.0f;
            if (glm::length(dir) > 0.001f) {
                dir = glm::normalize(dir);
                float turnSpeed = 10.0f * dt;
                rollDir = glm::mix(rollDir, dir, turnSpeed);
                if (glm::length(rollDir) > 0.9f) {
                    lastValidRollDir = rollDir;
                }
                glm::vec3 newPos = pos + dir * speed * dt;

                const float radius = 10.0f; // Arena yarıçapı
                const float wallThickness = 0.5f; // Duvar kalınlığı
                float dist = glm::length(glm::vec2(newPos.x, newPos.z));
                if (dist + 0.5f > radius - wallThickness) { // 0.5f küp yarıçapı
                    // Duvara çarptı, pozisyonu sınırda tut
                    float angle = atan2(newPos.z, newPos.x);
                    newPos.x = (radius - wallThickness - 0.5f) * cos(angle);
                    newPos.z = (radius - wallThickness - 0.5f) * sin(angle);
                }
                pos = newPos;

                rollTime += dt;
                if (rollTime > rollDur) {
                    rollTime = fmod(rollTime, rollDur);
                }
            }
        }
        else if (rolling) {
            rollTime += dt;
            if (rollTime >= stopRollDur) {
                rolling = false;
                rollTime = 0.0f;
            }
        }

        if (enableAbilities && dashing) {
            dashTime += dt;
            if (dashTime >= dashDur) {
                dashing = false;
                dashTime = 0.0f;
                dashCool = maxDashCool;
            }
            else {
                glm::vec3 dashDir = glm::normalize(glm::vec3(cam.front.x, 0.0f, cam.front.z));
                glm::vec3 newPos = pos + dashDir * speed * dashSpeed * dt;

                const float radius = 10.0f;
                const float wallThickness = 0.5f;
                float dist = glm::length(glm::vec2(newPos.x, newPos.z));
                if (dist + 0.5f > radius - wallThickness) {
                    float angle = atan2(newPos.z, newPos.x);
                    newPos.x = (radius - wallThickness - 0.5f) * cos(angle);
                    newPos.z = (radius - wallThickness - 0.5f) * sin(angle);
                }
                pos = newPos;
            }
        }

        if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_PRESS && !jumping) {
            jumping = true;
            jumpVel = jumpPower;
        }

        if (enableAbilities && glfwGetKey(win.window, GLFW_KEY_Q) == GLFW_PRESS && !jumping && superJumpCool <= 0.0f) {
            jumping = true;
            jumpVel = superJumpPower;
            superJumpCool = maxSuperJumpCool;
            superJumpUsed = true;
        }

        if (jumping) {
            pos.y += jumpVel * dt;
            jumpVel -= gravity * dt;
            if (pos.y <= 0.5f) {
                pos.y = 0.5f;
                jumping = false;
                jumpVel = 0.0f;
                if (enableAbilities && superJumpUsed) {
                    waveTriggered = true;
                    superJumpUsed = false;
                }
            }
        }

        if (enableAbilities && glfwGetKey(win.window, GLFW_KEY_E) == GLFW_PRESS && !dashing && !rolling && dashCool <= 0.0f) {
            dashing = true;
            dashTime = 0.0f;
        }

        if (enableAbilities && dashCool > 0.0f) dashCool -= dt;
        if (dashCool < 0.0f) dashCool = 0.0f;

        if (enableAbilities && superJumpCool > 0.0f) superJumpCool -= dt;
        if (superJumpCool < 0.0f) superJumpCool = 0.0f;
    }

    glm::mat4 getRollMatrix(const Camera& cam, const Window& win) {
        if (rolling) {
            float progress = rollTime / rollDur;
            float angle = glm::radians(360.0f * progress);
            glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), lastValidRollDir);
            if (glm::length(axis) < 0.1f) {
                axis = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            axis = glm::normalize(axis);
            glm::quat rot = glm::angleAxis(angle, axis);
            return glm::mat4_cast(rot);
        }
        return glm::mat4(1.0f);
    }

    float getDashCool() const { return dashCool; }
    float getSuperJumpCool() const { return superJumpCool; }
    bool getWaveTriggered() const { return waveTriggered; }
    void resetWave() { waveTriggered = false; }
};

class Wave {
public:
    glm::vec3 center;
    float radius;
    const float maxRadius = 5.0f;
    const float growSpeed = 5.0f;
    bool active;

    Wave() : radius(0.0f), active(false) {}

    void update(float dt) {
        if (active) {
            radius += growSpeed * dt;
            if (radius >= maxRadius) active = false;
        }
    }

    bool hit(const glm::vec3& pos) {
        float dist = glm::distance(center, pos);
        return active && dist <= radius;
    }

    glm::mat4 getModel() {
        return glm::translate(glm::mat4(1.0f), center) *
            glm::scale(glm::mat4(1.0f), glm::vec3(radius, 1.0f, radius));
    }
};

class AI {
public:
    glm::vec3 pos, vel;
    float rollAngle, speed, baseSpeed, stunTime;

    AI(glm::vec3 startPos) : pos(startPos), vel(0), rollAngle(0), speed(2.5f), baseSpeed(2.6f), stunTime(0) {}

    glm::mat4 getRollMatrix(Camera& cam) {
        return glm::rotate(glm::mat4(1.0f), rollAngle, glm::vec3(0, 0, 1));
    }

    void update(glm::vec3 playerPos, float dt, Wave& wave) {
        if (stunTime > 0) {
            stunTime -= dt;
            if (stunTime < 0) stunTime = 0;
            return;
        }

        glm::vec3 dir = playerPos - pos;
        float dist = glm::length(dir);
        dir = glm::normalize(dir);

        if (wave.active) {
            float waveDist = glm::length(wave.center - pos) - wave.radius;
            if (waveDist <= 0) {
                speed = 0;
                return;
            }
            else if (waveDist < 2.0f) {
                dir = -dir;
                speed = baseSpeed * 3.2f;
            }
        }

        speed = (dist < 2.0f) ? baseSpeed * 2.0f :
            (glm::length(playerPos - (pos - vel * dt)) > 5.0f) ? baseSpeed * 2.5f : baseSpeed;

        pos += dir * speed * dt;
        pos.y = 0.5f;

        float radius = 10.0f;
        float wallDist = glm::length(glm::vec2(pos.x, pos.z));
        if (wallDist + 0.5f > radius - 0.5f) {
            float angle = atan2(pos.z, pos.x);
            pos.x = (radius - 1.0f) * cos(angle);
            pos.z = (radius - 1.0f) * sin(angle);
        }

        rollAngle += 3.0f * dt;
    }
};

class Game {
public:
    Window win;
    Shader shader, waveShader, textShader, textureShader, uiShader, sliderShader;
    Renderer render;
    Camera cam;
    TextureMesh ground, wall, tribune, textureMesh, outerWall, spectatorCube;
    Mesh cube, waveMesh, sliderBarMesh, sliderHandleMesh;
    Texture dashTexture, superJumpTexture, arenaFloorTexture, arenaWallTexture, spectatorAreaTexture, SpectatorAreaUpperTexture, spectatorBlueTex, spectatorYellowTex, spectatorRedTex, spectatorGreenTex;
    Texture menuBackgroundTexture, playButtonTexture, hardnessTexture, story1Texture, story2Texture, story3Texture, story4Texture, settingsBackgroundTexture, keybindsTexture, lost1Texture;
    Texture lost2Texture, win1Texture, win2Texture, win3Texture, win4Texture, win5Texture;
    Player player;
    AI ai;
    Wave wave;
    float gameTime, mouseSensitivity, sliderValue, sliderMin, sliderMax, sliderX, sliderY, sliderWidth, sliderHeight, handleWidth, handleHeight;
    bool over, won, inMenu, inDifficultySelection, inStory, inSettings, mousePressed, spacePressed, dragging, inkeybinds;
    int currentStoryPage, lostStoryPage, winStoryPage;
    enum Difficulty { EASY, HARD };
    Difficulty currentDifficulty;
    TextRenderer* text;

    Game() : win(800, 600, "Catch Me If You Can"),
        shader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos;\n"
            "uniform mat4 model,view,projection; void main() { gl_Position=projection*view*model*vec4(aPos,1.0); }\n",
            // Fragment Shader
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec4 color; void main() { FragColor=color; }\n"
        ),
        waveShader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos;\n"
            "uniform mat4 model,view,projection; void main() { gl_Position=projection*view*model*vec4(aPos,1.0); }\n",
            // Fragment Shader
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec4 color; void main() { FragColor=color; }\n"
        ),
        textShader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec4 vertex; out vec2 TexCoords;\n"
            "uniform mat4 projection; void main() { gl_Position=projection*vec4(vertex.xy,0.0,1.0); TexCoords=vertex.zw; }\n",
            // Fragment Shader
            "#version 330 core\n"
            "in vec2 TexCoords; out vec4 color; uniform sampler2D text;\n"
            "uniform vec3 textColor; void main() { color=vec4(textColor,1.0)*vec4(1.0,1.0,1.0,texture(text,TexCoords).r); }\n"
        ),
        textureShader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos; layout(location=1) in vec2 aTexCoord; out vec2 TexCoord;\n"
            "uniform mat4 model,view,projection; void main() { gl_Position=projection*view*model*vec4(aPos,1.0); TexCoord=aTexCoord; }\n",
            // Fragment Shader
            "#version 330 core\n"
            "in vec2 TexCoord; out vec4 FragColor; uniform sampler2D texture1;\n"
            "uniform vec4 colorTint; void main() { FragColor=texture(texture1,TexCoord)*colorTint; }\n"
        ),
        uiShader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec2 aPos; layout(location=1) in vec2 aTexCoord; out vec2 TexCoord;\n"
            "uniform mat4 model,projection; void main() { gl_Position=projection*model*vec4(aPos,0.0,1.0); TexCoord=aTexCoord; }\n",
            // Fragment Shader
            "#version 330 core\n"
            "in vec2 TexCoord; out vec4 FragColor; uniform sampler2D texture1;\n"
            "uniform vec4 colorTint; void main() { FragColor=texture(texture1,TexCoord)*colorTint; }\n"
        ),
        sliderShader(
            // Vertex Shader
            "#version 330 core\n"
            "layout(location=0) in vec2 aPos;\n"
            "uniform mat4 model,projection; void main() { gl_Position=projection*model*vec4(aPos,0.0,1.0); }\n",
            // Fragment Shader
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec4 color; void main() { FragColor=color; }\n"
        ),
        render(),
        cam(win.w, win.h), textureMesh(), player(glm::vec3(0.0f, 0.5f, 0.0f), true), ai(glm::vec3(8.0f, 0.5f, 8.0f)), wave(), gameTime(0.0f), over(false), won(false), inMenu(true),
        ground(createGroundVerts(), createGroundInds()), cube(createCubeVerts(), createCubeInds()), waveMesh(createWaveVerts(), createWaveInds()), wall(createWallVertsWithUV(), createWallInds()),
        tribune(createTribuneVertsWithUV(), createTribuneInds()), outerWall(createOuterWallVertsWithUV(), createWallInds()), spectatorCube(createSpectatorCubeVerts(), createSpectatorCubeInds()),
        dashTexture("textures/dashfoto.png"), superJumpTexture("textures/superjumpfoto.png"), arenaFloorTexture("textures/ArenaFloor.png"), arenaWallTexture("textures/ArenaWall.png"),
        spectatorAreaTexture("textures/SpectatorArea.png"), SpectatorAreaUpperTexture("textures/SpectatorAreaUpper.png"), spectatorBlueTex("textures/SpectatorBlue.png"), spectatorYellowTex("textures/SpectatorYellow.png"),
        spectatorRedTex("textures/SpectatorRed.png"), spectatorGreenTex("textures/SpectatorGreen.png"), menuBackgroundTexture("textures/Mainmenu.png"), playButtonTexture("textures/playButton.png"),
        hardnessTexture("textures/Hardness.png"), story1Texture("textures/story1.png"), story2Texture("textures/story2.png"), story3Texture("textures/story3.png"), story4Texture("textures/story4.png"),
        settingsBackgroundTexture("textures/Settings.png"), keybindsTexture("textures/Keybinds.png"), lost1Texture("textures/Lost1.png"), lost2Texture("textures/Lost2.png"), win1Texture("textures/Win1.png"),
        win2Texture("textures/Win2.png"), win3Texture("textures/Win3.png"), win4Texture("textures/Win4.png"), win5Texture("textures/Win5.png"), winStoryPage(0), lostStoryPage(0),
        inDifficultySelection(false), inStory(false), inSettings(false), inkeybinds(false), currentStoryPage(0), mousePressed(false), spacePressed(false), currentDifficulty(EASY), text(new TextRenderer(&textShader)),
        mouseSensitivity(0.5f), sliderValue(0.5f), sliderMin(0.1f), sliderMax(1.0f), sliderX(790.0f), sliderY(880.0f), sliderWidth(300.0f), sliderHeight(20.0f), handleWidth(20.0f), handleHeight(20.0f), dragging(false),
        sliderBarMesh(createSliderBarVerts(), createSliderBarInds()), sliderHandleMesh(createSliderHandleVerts(), createSliderHandleInds()) {

        glfwSetInputMode(win.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~Game() { delete text; }

    vector<GLfloat> createGroundVerts() {
        vector<GLfloat> verts = { 0.0f, 0.0f, 0.0f, 0.5f, 0.5f };
        const int segments = 32;
        const float radius = 10.0f, pi2 = 2.0f * glm::pi<float>();
        for (int i = 0; i <= segments; ++i) {
            float angle = pi2 * i / segments, c = cos(angle), s = sin(angle);
            verts.insert(verts.end(), { radius * c, 0.0f, radius * s, (c + 1.0f) * 0.5f, (s + 1.0f) * 0.5f });
        }
        return verts;
    }

    vector<GLfloat> createWallVertsWithUV() {
        vector<GLfloat> verts;
        const int segments = 32;
        const float radius = 10.0f, height = 5.0f, thickness = 0.5f, pi2 = 2.0f * glm::pi<float>();
        for (int i = 0; i <= segments; ++i) {
            float angle = pi2 * i / segments, c = cos(angle), s = sin(angle), u = i / (float)segments;
            float xo = radius * c, zo = radius * s, xi = (radius - thickness) * c, zi = (radius - thickness) * s;
            verts.insert(verts.end(), { xo, -height / 2, zo, u, 0.0f, xo, height / 2, zo, u, 1.0f, xi, -height / 2, zi, u, 0.0f, xi, height / 2, zi, u, 1.0f });
        }
        return verts;
    }

    vector<GLfloat> createTribuneVertsWithUV() {
        vector<GLfloat> verts;
        const int segments = 64;
        const float radius = 10.0f, wallHeight = 4.7f, tribuneDepth = 3.0f, pi2 = 2.0f * glm::pi<float>();
        for (int i = 0; i <= segments; ++i) {
            float angle = pi2 * i / segments, c = cos(angle), s = sin(angle), u = i / (float)segments;
            verts.insert(verts.end(), { radius * c, wallHeight, radius * s, u, 0.0f, (radius + tribuneDepth) * c, wallHeight, (radius + tribuneDepth) * s, u, 1.0f });
        }
        return verts;
    }

    vector<GLfloat> createOuterWallVertsWithUV() {
        vector<GLfloat> verts;
        const int segments = 32;
        const float radius = 13.0f, height = 10.0f, thickness = 0.5f, pi2 = 2.0f * glm::pi<float>();
        for (int i = 0; i <= segments; ++i) {
            float angle = pi2 * i / segments, c = cos(angle), s = sin(angle), u = i / (float)segments;
            float xo = radius * c, zo = radius * s, xi = (radius - thickness) * c, zi = (radius - thickness) * s;
            verts.insert(verts.end(), { xo, 0.0f, zo, u, 0.0f, xo, height, zo, u, 1.0f, xi, 0.0f, zi, u, 0.0f, xi, height, zi, u, 1.0f });
        }
        return verts;
    }

    vector<GLfloat> createSpectatorCubeVerts() {
        return {
            -0.5f,-0.5f,-0.5f,0.0f,0.0f, 0.5f,-0.5f,-0.5f,1.0f,0.0f, 0.5f,0.5f,-0.5f,1.0f,1.0f, -0.5f,0.5f,-0.5f,0.0f,1.0f,
            -0.5f,-0.5f,0.5f,0.0f,0.0f, 0.5f,-0.5f,0.5f,1.0f,0.0f, 0.5f,0.5f,0.5f,1.0f,1.0f, -0.5f,0.5f,0.5f,0.0f,1.0f,
            -0.5f,0.5f,-0.5f,0.0f,0.0f, 0.5f,0.5f,-0.5f,1.0f,0.0f, 0.5f,0.5f,0.5f,1.0f,1.0f, -0.5f,0.5f,0.5f,0.0f,1.0f,
            -0.5f,-0.5f,-0.5f,0.0f,0.0f, 0.5f,-0.5f,-0.5f,1.0f,0.0f, 0.5f,-0.5f,0.5f,1.0f,1.0f, -0.5f,-0.5f,0.5f,0.0f,1.0f,
            0.5f,-0.5f,-0.5f,0.0f,0.0f, 0.5f,0.5f,-0.5f,1.0f,0.0f, 0.5f,0.5f,0.5f,1.0f,1.0f, 0.5f,-0.5f,0.5f,0.0f,1.0f,
            -0.5f,-0.5f,-0.5f,0.0f,0.0f, -0.5f,0.5f,-0.5f,1.0f,0.0f, -0.5f,0.5f,0.5f,1.0f,1.0f, -0.5f,-0.5f,0.5f,0.0f,1.0f
        };
    }

    vector<GLuint> createSpectatorCubeInds() {
        vector<GLuint> inds;
        for (GLuint i = 0; i < 6; ++i) inds.insert(inds.end(), { i * 4, i * 4 + 1, i * 4 + 2, i * 4, i * 4 + 2, i * 4 + 3 });
        return inds;
    }

    vector<GLuint> createGroundInds() {
        vector<GLuint> inds;
        const GLuint segments = 32;
        for (GLuint i = 0; i < segments; ++i) inds.insert(inds.end(), { 0, i + 1, i + 2 });
        inds.insert(inds.end(), { 0, segments + 1, 1 });
        return inds;
    }

    vector<GLuint> createTribuneInds() {
        vector<GLuint> inds;
        const GLuint segments = 64;
        for (GLuint i = 0; i < segments; ++i) {
            GLuint b = i * 2;
            inds.insert(inds.end(), { b, b + 1, b + 2, b + 2, b + 1, b + 3 });
        }
        return inds;
    }

    vector<GLuint> createWallInds() {
        vector<GLuint> inds;
        const GLuint segments = 32;
        for (GLuint i = 0; i < segments; ++i) {
            GLuint b = i * 4;
            inds.insert(inds.end(), { b,b + 1,b + 5, b + 5,b + 4,b, b + 2,b + 6,b + 3, b + 6,b + 7,b + 3, b + 1,b + 3,b + 7, b + 7,b + 5,b + 1, b,b + 4,b + 2, b + 4,b + 6,b + 2 });
        }
        return inds;
    }

    vector<GLfloat> createCubeVerts() {
        return { -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f,
                -0.5f,-0.5f,0.5f, 0.5f,-0.5f,0.5f, 0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f };
    }

    vector<GLuint> createCubeInds() {
        return { 0,1,2,2,3,0, 1,5,6,6,2,1, 5,4,7,7,6,5, 4,0,3,3,7,4, 3,2,6,6,7,3, 0,4,5,5,1,0 };
    }

    vector<GLfloat> createWaveVerts() {
        vector<GLfloat> verts;
        const int segments = 64;
        const float radius = 1.0f, thickness = 0.1f, pi2 = 2.0f * glm::pi<float>();
        for (int i = 0; i <= segments; ++i) {
            float angle = pi2 * i / segments, c = cos(angle), s = sin(angle);
            verts.insert(verts.end(), { (radius - thickness) * c, 0.0f, (radius - thickness) * s, radius * c, 0.0f, radius * s });
        }
        return verts;
    }

    vector<GLuint> createWaveInds() {
        vector<GLuint> inds;
        const GLuint segments = 64;
        for (GLuint i = 0; i < segments; ++i) {
            GLuint b = i * 2;
            inds.insert(inds.end(), { b, b + 1, b + 2, b + 2, b + 1, b + 3 });
        }
        return inds;
    }

    vector<GLfloat> createSliderBarVerts() {
        return { 0.0f,0.0f,0.0f, 300.0f,0.0f,0.0f, 300.0f,20.0f,0.0f, 0.0f,20.0f,0.0f };
    }

    vector<GLuint> createSliderBarInds() {
        return { 0,1,2, 2,3,0 };
    }

    vector<GLfloat> createSliderHandleVerts() {
        return { -10.0f,-10.0f,0.0f, 10.0f,-10.0f,0.0f, 10.0f,10.0f,0.0f, -10.0f,10.0f,0.0f };
    }

    vector<GLuint> createSliderHandleInds() {
        return { 0,1,2, 2,3,0 };
    }

    void drawSpectators(float dt) {
        const int count = 50;
        const float radius = 11.5f, baseHeight = 5.7f, minDist = 2.0f, pi2 = 2.0f * glm::pi<float>();
        static vector<glm::vec3> positions;
        static vector<float> jumpPhases;
        static vector<Texture*> textures;

        if (positions.empty()) {
            srand(static_cast<unsigned>(time(nullptr)));
            for (int i = 0; i < count; ++i) {
                glm::vec3 pos;
                for (int j = 0; j < 100; ++j) {
                    float angle = static_cast<float>(rand()) / RAND_MAX * pi2;
                    float offset = -0.5f + static_cast<float>(rand()) / RAND_MAX;
                    pos = { radius * cos(angle) + offset, baseHeight, radius * sin(angle) + offset };
                    bool valid = true;
                    for (const auto& p : positions) {
                        if (glm::distance(pos, p) < minDist) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid) {
                        positions.push_back(pos);
                        jumpPhases.push_back(static_cast<float>(rand()) / RAND_MAX);
                        int tex = rand() % 4;
                        textures.push_back(tex == 0 ? &spectatorBlueTex : tex == 1 ? &spectatorYellowTex : tex == 2 ? &spectatorRedTex : &spectatorGreenTex);
                        break;
                    }
                }
            }
        }

        static float jumpTime = 0.0f;
        jumpTime += dt * 2.0f;
        for (size_t i = 0; i < positions.size(); ++i) {
            float jump = sin((jumpPhases[i] + jumpTime) * pi2) * 0.2f;
            glm::vec3 pos = positions[i];
            pos.y = baseHeight + jump;
            float dist = glm::length(player.pos - pos);
            if (dist < 5.0f) {
                jump += sin((jumpPhases[i] + jumpTime) * pi2 * 2.0f) * 0.3f * (1.0f - dist / 5.0f);
                pos.y = baseHeight + jump;
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::scale(model, glm::vec3(1.2f));
            glm::vec3 toCenter = glm::normalize(-pos);
            model = glm::rotate(model, atan2(toCenter.z, toCenter.x) + pi2 / 2.0f, { 0.0f, 1.0f, 0.0f });
            render.drawTexture(spectatorCube, textureShader, *textures[i], model, cam.view, cam.proj, glm::vec4(1.0f), false);
        }
    }

    void run() {
        double lastTime = glfwGetTime(), lastFTime = 0.0;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        bool fullscreen = false;

        while (!win.shouldClose()) {
            float dt = static_cast<float>(glfwGetTime() - lastTime);
            lastTime = glfwGetTime();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Fullscreen toggle
            if (glfwGetKey(win.window, GLFW_KEY_F) == GLFW_PRESS && glfwGetTime() - lastFTime > 0.5) {
                lastFTime = glfwGetTime();
                fullscreen = !fullscreen;
                if (fullscreen) {
                    glfwSetWindowMonitor(win.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    glViewport(0, 0, win.w = cam.w = mode->width, win.h = cam.h = mode->height);
                }
                else {
                    glfwSetWindowMonitor(win.window, nullptr, 100, 100, 800, 600, 0);
                    glViewport(0, 0, win.w = cam.w = 800, win.h = cam.h = 600);
                }
                cam.proj = glm::perspective(glm::radians(45.0f), (float)win.w / win.h, 0.1f, 100.0f);
            }

            glm::mat4 proj = glm::ortho(0.0f, (float)win.w, 0.0f, (float)win.h);
            auto drawUI = [&](Texture& tex, float x, float y, float w, float h, bool interact = false, std::function<void()> action = [] {}) {
                glm::mat4 model = interact ? glm::mat4(0.0f) : // Butonlar için sıfır matris (görünmez)
                    glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), glm::vec3(w / 250.0f, h / 250.0f, 1.0f));
                uiShader.use();
                tex.bind(GL_TEXTURE0);
                render.drawTexture(textureMesh, uiShader, tex, model, glm::mat4(1.0f), proj, glm::vec4(1.0f), true);
                if (interact && glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mousePressed) {
                    double mx, my;
                    glfwGetCursorPos(win.window, &mx, &my);
                    my = win.h - my;
                    if (mx >= x - w / 2 && mx <= x + w / 2 && my >= y - h / 2 && my <= y + h / 2) {
                        mousePressed = true;
                        action();
                    }
                }
                };

            if (inMenu) {
                glDisable(GL_DEPTH_TEST);
                drawUI(menuBackgroundTexture, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                drawUI(playButtonTexture, win.w / 2.0f - 9.8f, win.h / 2.0f + 188.2f, 640.2f, 92.0f, true, [&] { inMenu = false; inDifficultySelection = true; });
                drawUI(playButtonTexture, win.w / 2.0f - 9.8f, win.h / 2.0f + 41.2f, 640.2f, 92.0f, true, [&] { inMenu = false; inSettings = true; });
                drawUI(playButtonTexture, win.w / 2.0f - 9.8f, win.h / 2.0f - 195.0f, 640.2f, 92.0f, true, [&] { glfwDestroyWindow(win.window); glfwTerminate(); exit(0); });
                if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) mousePressed = false;
                glEnable(GL_DEPTH_TEST);
            }
            else if (inkeybinds) {
                glDisable(GL_DEPTH_TEST);
                drawUI(keybindsTexture, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                drawUI(playButtonTexture, win.w / 2.0f + 40.5f, win.h / 2.0f - 353.0f, 626.2f, 88.0f, true, [&] { inkeybinds = false; inSettings = true; });
                if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) mousePressed = false;
                glEnable(GL_DEPTH_TEST);
            }
            else if (inSettings) {
                glDisable(GL_DEPTH_TEST);
                drawUI(settingsBackgroundTexture, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                // Slider bar
                glm::mat4 sliderModel = glm::translate(glm::mat4(1.0f), glm::vec3(sliderX, sliderY, 0.0f));
                sliderShader.use();
                sliderShader.setMat4("model", sliderModel);
                sliderShader.setMat4("projection", proj);
                sliderShader.setVec4("color", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
                render.draw(sliderBarMesh, sliderShader, glm::mat4(1.0f), proj, sliderModel, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
                // Slider handle
                float handleX = sliderX + (sliderValue * sliderWidth) - handleWidth / 2.0f;
                glm::mat4 handleModel = glm::translate(glm::mat4(1.0f), glm::vec3(handleX, sliderY + 10.1f, 1.0f));
                sliderShader.setMat4("model", handleModel);
                sliderShader.setMat4("projection", proj);
                sliderShader.setVec4("color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                render.draw(sliderHandleMesh, sliderShader, glm::mat4(1.0f), proj, handleModel, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                // Buttons
                drawUI(playButtonTexture, win.w / 2.0f - 32.5f, win.h / 2.0f - 210.0f, 626.2f, 88.0f, true, [&] { inSettings = false; inMenu = true; });
                drawUI(playButtonTexture, win.w / 2.0f - 32.0f, 618.0f, 626.0f, 88.0f, true, [&] { inkeybinds = true; inSettings = false; });
                // Fullscreen buttons
                double mx, my;
                glfwGetCursorPos(win.window, &mx, &my);
                my = win.h - my;
                auto drawColorButton = [&](float x, float y, float w, float h, glm::vec4 color, std::function<void()> action) {
                    glm::mat4 model = glm::mat4(0.0f); // Görünmez yapmak için sıfır matris
                    sliderShader.use();
                    sliderShader.setMat4("model", model);
                    sliderShader.setMat4("projection", proj);
                    sliderShader.setVec4("color", color);
                    render.draw(sliderBarMesh, sliderShader, glm::mat4(1.0f), proj, model, color);
                    if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mousePressed &&
                        mx >= x - w / 2 && mx <= x + w / 2 && my >= y - h / 2 && my <= y + h / 2) {
                        mousePressed = true;
                        action();
                    }
                    };
                drawColorButton(666.5f, 715.0f, 147.0f, 920.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), [&] {
                    glfwSetWindowMonitor(win.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    glViewport(0, 0, win.w = cam.w = mode->width, win.h = cam.h = mode->height);
                    cam.proj = glm::perspective(glm::radians(45.0f), (float)win.w / win.h, 0.1f, 100.0f);
                    });
                drawColorButton(1015.5f, 715.0f, 147.0f, 100.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), [&] {
                    glfwSetWindowMonitor(win.window, nullptr, 100, 100, 800, 600, 0);
                    glViewport(0, 0, win.w = cam.w = 800, win.h = cam.h = 600);
                    cam.proj = glm::perspective(glm::radians(45.0f), (float)win.w / win.h, 0.1f, 100.0f);
                    });
                // Slider interaction
                if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mousePressed &&
                    mx >= sliderX && mx <= sliderX + sliderWidth && my >= sliderY - sliderHeight / 2.0f && my <= sliderY + sliderHeight / 2.0f) {
                    mousePressed = dragging = true;
                }
                if (dragging) {
                    sliderValue = glm::clamp((float)(mx - sliderX) / sliderWidth, (float)sliderMin, (float)sliderMax);
                    cam.mouseSensitivity = sliderValue;
                }
                if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) mousePressed = dragging = false;
                glEnable(GL_DEPTH_TEST);
            }
            else if (inDifficultySelection) {
                glDisable(GL_DEPTH_TEST);
                drawUI(hardnessTexture, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                drawUI(playButtonTexture, win.w / 2.0f - 24.5f, win.h / 2.0f + 162.2f, 644.2f, 92.0f, true, [&] {
                    currentDifficulty = EASY; player.enableAbilities = true; inDifficultySelection = false; inStory = true; currentStoryPage = 1;
                    });
                drawUI(playButtonTexture, win.w / 2.0f - 24.5f, win.h / 2.0f + 16.8f, 644.2f, 92.0f, true, [&] {
                    currentDifficulty = HARD; player.enableAbilities = false; inDifficultySelection = false; inStory = true; currentStoryPage = 1;
                    });
                drawUI(playButtonTexture, win.w / 2.0f - 26.0f, win.h / 2.0f - 218.0f, 640.2f, 88.0f, true, [&] { inDifficultySelection = false; inMenu = true; });
                if (glfwGetMouseButton(win.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) mousePressed = false;
                glEnable(GL_DEPTH_TEST);
            }
            else if (inStory) {
                glDisable(GL_DEPTH_TEST);
                Texture* storyTex = nullptr;
                if (currentStoryPage == 1) storyTex = &story1Texture;
                else if (currentStoryPage == 2) storyTex = &story2Texture;
                else if (currentStoryPage == 3) storyTex = &story3Texture;
                else if (currentStoryPage == 4) storyTex = &story4Texture;
                if (storyTex) drawUI(*storyTex, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
                    spacePressed = true;
                    if (++currentStoryPage > 4) {
                        inStory = false;
                        glfwSetInputMode(win.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        cam.firstMouse = true;
                    }
                }
                if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_RELEASE) spacePressed = false;
                glEnable(GL_DEPTH_TEST);
            }
            else if (!over) {
                gameTime += dt;
                double mx, my;
                glfwGetCursorPos(win.window, &mx, &my);
                cam.mouseInput(win, mx, my);
                cam.mouseSensitivity = mouseSensitivity;
                player.update(win, cam, dt);
                ai.update(player.pos, dt, wave);
                cam.setTarget(player.pos);
                cam.updateView(dt);

                if (player.getWaveTriggered()) {
                    wave.center = player.pos;
                    wave.radius = 0.0f;
                    wave.active = true;
                    player.resetWave();
                }
                if (wave.active && glm::length(wave.center - ai.pos) <= wave.radius) ai.stunTime = 2.0f;
                wave.update(dt);

                auto draw3D = [&](TextureMesh& mesh, Texture& tex, glm::mat4 model, glm::vec4 color = glm::vec4(1.0f)) {
                    textureShader.use();
                    render.drawTexture(mesh, textureShader, tex, model, cam.view, cam.proj, color, false);
                    };
                draw3D(ground, arenaFloorTexture, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)));
                shader.use();
                render.draw(cube, shader, cam.view, cam.proj, glm::translate(glm::mat4(1.0f), player.pos) * player.getRollMatrix(cam, win), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                render.draw(cube, shader, cam.view, cam.proj, glm::translate(glm::mat4(1.0f), ai.pos) * ai.getRollMatrix(cam), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                for (int i = 0; i < 32; ++i) draw3D(wall, arenaWallTexture, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, 0.0f)));
                draw3D(tribune, spectatorAreaTexture, glm::mat4(1.0f));
                drawSpectators(dt);
                for (int i = 0; i < 32; ++i) draw3D(outerWall, SpectatorAreaUpperTexture, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
                if (player.enableAbilities && wave.active) {
                    glDisable(GL_DEPTH_TEST);
                    waveShader.use();
                    waveShader.setMat4("model", wave.getModel());
                    waveShader.setMat4("view", cam.view);
                    waveShader.setMat4("projection", cam.proj);
                    waveShader.setVec4("color", glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
                    render.draw(waveMesh, waveShader, cam.view, cam.proj, wave.getModel(), glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
                    glEnable(GL_DEPTH_TEST);
                }
                if (glm::length(player.pos - ai.pos) < 1.15f) {
                    over = true;
                    won = false;
                    cout << "Yakalandın!\n";
                }
                else if (gameTime >= 60.0f) {
                    over = true;
                    won = true;
                    cout << "Kazandın! 60 saniye hayatta kaldın!\n";
                }
                glDisable(GL_DEPTH_TEST);
                textShader.use();
                textShader.setMat4("projection", proj);
                int timeLeft = std::max(0, static_cast<int>(60.0f - gameTime));
                text->draw("Time: " + to_string(timeLeft) + "s", win.w - 200.0f, win.h - 50.0f, 0.5f, glm::vec3(1.0f));
                if (player.enableAbilities) {
                    auto drawDashAbility = [&](Texture& tex, float x, float y, float cool, string abilityText) {
                        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
                        uiShader.use();
                        tex.bind(GL_TEXTURE0);
                        render.drawTexture(textureMesh, uiShader, tex, model, glm::mat4(1.0f), proj, cool > 0.0f ? glm::vec4(0.2f) : glm::vec4(1.0f), true);
                        text->draw(abilityText, x + 125.0f - abilityText.length() * 60.0f, y - 105.0f, 0.5f, glm::vec3(1.0f));
                        };
                    auto drawSuperJumpAbility = [&](Texture& tex, float x, float y, float cool, string abilityText) {
                        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
                        uiShader.use();
                        tex.bind(GL_TEXTURE0);
                        render.drawTexture(textureMesh, uiShader, tex, model, glm::mat4(1.0f), proj, cool > 0.0f ? glm::vec4(0.2f) : glm::vec4(1.0f), true);
                        text->draw(abilityText, x + 110.0f - abilityText.length() * 60.0f, y - 135.0f, 0.5f, glm::vec3(1.0f));
                        };
                    drawDashAbility(dashTexture, win.w / 2.0f - 215.0f, 135.0f, player.getDashCool(), to_string(static_cast<int>(player.getDashCool())) + "s");
                    drawSuperJumpAbility(superJumpTexture, win.w / 2.0f + 205.0f, 165.0f, player.getSuperJumpCool(), to_string(static_cast<int>(player.getSuperJumpCool())) + "s");
                }
                glEnable(GL_DEPTH_TEST);
            }
            else {
                glDisable(GL_DEPTH_TEST);
                Texture* tex = nullptr;
                if (won) {
                    if (winStoryPage == 0) tex = &win1Texture;
                    else if (winStoryPage == 1) tex = &win2Texture; else if (winStoryPage == 2) tex = &win3Texture; else if (winStoryPage == 3) tex = &win4Texture; else if (winStoryPage == 4) tex = &win5Texture;
                    if (winStoryPage == 4) text->draw("Press R to Restart", (win.w - 270.0f) / 2.0f, 50.0f, 0.5f, glm::vec3(1.0f));
                    if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
                        spacePressed = true;
                        if (++winStoryPage > 4) winStoryPage = 4;
                    }
                    if (winStoryPage == 4 && glfwGetKey(win.window, GLFW_KEY_R) == GLFW_PRESS) {
                        player.pos = glm::vec3(0.0f, 0.5f, 0.0f);
                        ai.pos = glm::vec3(8.0f, 0.5f, 8.0f);
                        gameTime = 0.0f;
                        over = won = false;
                        inMenu = true;
                        winStoryPage = 0;
                        glfwSetInputMode(win.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        cam.firstMouse = true;
                    }
                }
                else {
                    tex = lostStoryPage == 0 ? &lost1Texture : &lost2Texture;
                    if (lostStoryPage == 1) text->draw("Press R to Restart", (win.w - 270.0f) / 2.0f, 50.0f, 0.5f, glm::vec3(1.0f));
                    if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
                        spacePressed = true;
                        if (++lostStoryPage > 1) lostStoryPage = 1;
                    }
                    if (lostStoryPage == 1 && glfwGetKey(win.window, GLFW_KEY_R) == GLFW_PRESS) {
                        player.pos = glm::vec3(0.0f, 0.5f, 0.0f);
                        ai.pos = glm::vec3(8.0f, 0.5f, 8.0f);
                        gameTime = 0.0f;
                        over = won = false;
                        inMenu = true;
                        lostStoryPage = 0;
                        glfwSetInputMode(win.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        cam.firstMouse = true;
                    }
                }
                if (tex) drawUI(*tex, win.w / 2.0f, win.h / 2.0f, (float)win.w, (float)win.h);
                if (glfwGetKey(win.window, GLFW_KEY_SPACE) == GLFW_RELEASE) spacePressed = false;
                glEnable(GL_DEPTH_TEST);
            }
            win.swap();
            win.poll();
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}