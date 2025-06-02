#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube daySkybox;
uniform samplerCube duskSkybox;
uniform samplerCube nightSkybox;
uniform float dayNightCycle;

void main() {
    vec4 dayColor = texture(daySkybox, TexCoords);
    vec4 duskColor = texture(duskSkybox, TexCoords);
    vec4 nightColor = texture(nightSkybox, TexCoords);

    // Define transition ranges
    float dayRange = 0.5; // Day: 0.0 to 0.5 (50%)
    float duskRange = 0.75; // Dusk: 0.5 to 0.75 (25%)

    vec4 finalColor;
    if (dayNightCycle < dayRange) {
        // Transition from day to dusk
        float t = dayNightCycle / dayRange; // Normalize to [0, 1]
        finalColor = mix(dayColor, duskColor, t);
    } else if (dayNightCycle < duskRange) {
        // Transition from dusk to night
        float t = (dayNightCycle - dayRange) / (duskRange - dayRange); // Normalize to [0, 1]
        finalColor = mix(duskColor, nightColor, t);
    } else {
        // Night phase
        finalColor = nightColor;
    }

    // Optional: Mask bottom face to black (if not visible)
    if (TexCoords.y < -0.9) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black for bottom face
    } else {
        FragColor = finalColor;
    }
}