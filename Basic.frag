#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform bool useTexture;
uniform vec3 myColor;
uniform bool flash; // <--- ΝΕΑ ΜΕΤΑΒΛΗΤΗ

void main() {
    vec4 result;
    
    if (useTexture) {
        result = texture(texture1, TexCoord);
    } else {
        result = vec4(myColor, 1.0);
    }

    // Αν το flash είναι true, το χρώμα γίνεται κάτασπρο
    if (flash) {
        FragColor = vec4(1.0, 1.0, 1.0, 0.1); 
    } else {
        FragColor = result;
    }
}