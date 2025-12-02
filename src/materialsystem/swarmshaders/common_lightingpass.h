#define COMMON_LIGHTINGPASS_H
#ifdef COMMON_LIGHTINGPASS_H
ConVar cl_light_specular_factor("cl_light_specular_factor", "7", FCVAR_CHEAT);
ConVar cl_light_specular_spot_boost("cl_light_specular_spot_boost", "0.01", FCVAR_CHEAT); // was .1 // was .43
ConVar cl_light_specular_point_boost("cl_light_specular_point_boost", "0.01", FCVAR_CHEAT); //was .1 // was .43
ConVar cl_light_specular_grazing_factor("cl_light_specular_grazing_factor", "0", FCVAR_CHEAT); // values above one cause perfect mirror at direct angles.
ConVar cl_light_specular_grazing_power("cl_light_specular_grazing_power", "0", FCVAR_CHEAT);	// values above one cause perfect mirror at direct angles.
ConVar cl_light_specular_spot_size("cl_light_specular_spot_size", "0.001", FCVAR_CHEAT);
ConVar cl_light_specular_point_size("cl_light_specular_point_size", "0.001", FCVAR_CHEAT);
ConVar cl_light_specular_brightness_spot("cl_light_specular_brightness_spot", "25.0", FCVAR_CHEAT);
ConVar cl_light_specular_scale("cl_light_specular_scale", "2", FCVAR_CHEAT);
ConVar cl_light_diffuse_strength_point("cl_light_diffuse_strength_point", "0.01", FCVAR_CHEAT);
ConVar cl_light_diffuse_strength_spot("cl_light_diffuse_strength_spot", "0.01", FCVAR_CHEAT);
ConVar cl_light_fresnel_strength("cl_light_fresnel_strength", "10.0", FCVAR_CHEAT);
ConVar cl_light_Sheen_strength("cl_light_Sheen_strength", "0.01", FCVAR_CHEAT);
ConVar cl_light_ibl_intensity("cl_light_ibl_intensity", "0.5", FCVAR_CHEAT);
ConVar cl_shadow_filter_type("cl_shadow_filter_type", "1", FCVAR_CHEAT); // 0 = hard, 1 = soft shadows / pcss

//make a system that detects if mrao is defined then resort to making mrao render with dedicated texture
//otherwise we use auto generation thus making the value for 1.86 to 10 when dedicated is detected.
//ConVar cl_light_MRAO_green("cl_light_MRAO_green", "1.86"); // defualt for MRAO dedicated textures is 10.
ConVar cl_light_MRAO_blue("cl_light_MRAO_blue", "2.0");
#endif