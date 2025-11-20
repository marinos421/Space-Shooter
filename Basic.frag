#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// Μεταβλητές που στέλνουμε από την C++
uniform sampler2D texture1; // Η εικόνα
uniform bool useTexture;    // Διακόπτης: true = εικόνα, false = χρώμα
uniform vec3 myColor;       // Το χρώμα (αν δεν βάλουμε εικόνα)

void main() {
    if (useTexture) {
        // Αν είναι ο παίκτης, βάλε την υφή
        FragColor = texture(texture1, TexCoord);
    } else {
        // Αν είναι εχθρός ή σφαίρα, βάλε το χρώμα (myColor)
        FragColor = vec4(myColor, 1.0);
    }
}