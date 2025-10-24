#pragma once
#include<string>

const std::string DefaultVertexShader =
R"(
		#version 130
		in vec2 position;
		//in vec2 texCoord;

		//out vec2 v_TexCoord;


		void main()
		{
			gl_Position = vec4(position,0,1);
			//v_TexCoord = texCoord;
		}
   )";


const std::string FragmentShaderHeader =
R"(
				#version 130
                #ifdef GL_ES
                precision mediump float;
                #endif
 out vec4 color;

//in vec2 v_TexCoord; 

                uniform vec2      iResolution;           // viewport resolution (in pixels)
                uniform float     iDuration;             // shader playback total time (in seconds)
                uniform float     iTime;                 // shader playback current time (in seconds)
                uniform float     iTimeDelta;            // render time (in seconds)
                uniform int       iFrame;                // shader playback FilterFrame
                uniform float     iChannelTime[8];       // channel playback time (in seconds)
                uniform vec3      iChannelResolution[8]; // channel resolution (in pixels)
                uniform sampler2D iChannel[8];           // input channel. XX = 2D/Cube
				uniform sampler3D iChannel_3D;			 // use for 3D texture



                //uniform float iProgress =  iTime / iDuration;
				float iProgress =  iTime / iDuration;
                                
				float iAspect = iResolution.x / iResolution.y;

                float global_time;
				float PREFIX(float aa)
				{
					return  iTime / iDuration; 
				}

                float iGlobalTime =  iProgress; 
				const float PI = 3.141592653589793;

				vec4 INPUT1(vec2 position)
				{
					vec2 p = gl_FragCoord.xy / iResolution.xy;
  					return texture2D(iChannel[0], position);
				}

				vec4 INPUT2(vec2 position)
				{
					vec2 p = gl_FragCoord.xy / iResolution.xy;
  					return texture2D(iChannel[1], position);
				}
				vec4 FUNCNAME(vec2 tc); 

            )";