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
    float dayRange = 0.33; // Day: 0.0 to 0.33
    float duskRange = 0.66; // Dusk: 0.33 to 0.66
    // Night: 0.66 to 1.0

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
        // Night phase (no transition after 0.66)
        finalColor = nightColor;
    }

    FragColor = finalColor;
}