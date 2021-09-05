#include "RaidenMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

RaidenMode::RaidenMode() {

	//set up trail as if ball has been here for 'forever':
	ball_trail.clear();
	ball_trail.emplace_back(ball, trail_length);
	ball_trail.emplace_back(ball, 0.0f);

	
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

RaidenMode::~RaidenMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool RaidenMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	switch (evt.key.keysym.sym) {
	case SDLK_UP:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_up : curr_status & ~EventStatus::is_up;
		break;
	case SDLK_DOWN:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_down : curr_status & ~EventStatus::is_down;
		break;
	case SDLK_LEFT:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_left : curr_status & ~EventStatus::is_left;
		break;
	case SDLK_RIGHT:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_right : curr_status & ~EventStatus::is_right;
		break;
	case SDLK_SPACE:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status | EventStatus::is_shoot : curr_status & ~EventStatus::is_shoot;
		break;
	}

	return false;
}

void RaidenMode::execute_event(float elapsed) {

	glm::vec2 movement(0);
	if (curr_status & EventStatus::is_down)
		movement.y -= bot_fighter_speed.y * elapsed;
	if (curr_status & EventStatus::is_up)
		movement.y += bot_fighter_speed.y * elapsed;
	if (curr_status & EventStatus::is_left)
		movement.x -= bot_fighter_speed.x * elapsed;
	if (curr_status & EventStatus::is_right)
		movement.x += bot_fighter_speed.x * elapsed;
	if (curr_status & EventStatus::is_shoot) {
		if (curr_player_shoot_cool_down > 0)
		{
			curr_player_shoot_cool_down -= elapsed;
		}
		else
		{
			player_shoot();
			curr_player_shoot_cool_down = player_shoot_cool_down;
		}
	}
	glm::normalize(movement);
	bot_fighter += movement;
}

void RaidenMode::player_shoot() {
	Bullet b(glm::vec3(bot_fighter.x, bot_fighter.y + fighter_radius.y + 0.05f, 0), 7.0f);
	all_bullets.push_back(b);
}

void RaidenMode::update_bullet(float elapsed) {
	for (auto& b : all_bullets)
	{
		b.bullet_position.y += b.bullet_speed * b.shoot_dir * elapsed;
	}
}

void RaidenMode::update(float elapsed) {

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	// Execute Keyboard event
	execute_event(elapsed);

	// Update bullet
	update_bullet(elapsed);

	//clamp fighters to court:
	bot_fighter.y = std::max(bot_fighter.y, -court_radius.y + fighter_radius.y);
	bot_fighter.y = std::min(bot_fighter.y, court_radius.y - fighter_radius.y);

	//---- collision handling ----

	//paddles:
	auto paddle_vs_ball = [this](glm::vec2 const &paddle) {
		//compute area of overlap:
		glm::vec2 min = glm::max(paddle - paddle_radius, ball - ball_radius);
		glm::vec2 max = glm::min(paddle + paddle_radius, ball + ball_radius);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return;

		if (max.x - min.x > max.y - min.y) {
			//wider overlap in x => bounce in y direction:
			if (ball.y > paddle.y) {
				ball.y = paddle.y + paddle_radius.y + ball_radius.y;
				ball_velocity.y = std::abs(ball_velocity.y);
			} else {
				ball.y = paddle.y - paddle_radius.y - ball_radius.y;
				ball_velocity.y = -std::abs(ball_velocity.y);
			}
		} else {
			//wider overlap in y => bounce in x direction:
			if (ball.x > paddle.x) {
				ball.x = paddle.x + paddle_radius.x + ball_radius.x;
				ball_velocity.x = std::abs(ball_velocity.x);
			} else {
				ball.x = paddle.x - paddle_radius.x - ball_radius.x;
				ball_velocity.x = -std::abs(ball_velocity.x);
			}
			//warp y velocity based on offset from paddle center:
			float vel = (ball.y - paddle.y) / (paddle_radius.y + ball_radius.y);
			ball_velocity.y = glm::mix(ball_velocity.y, vel, 0.75f);
		}
	};

	//court walls:
	if (ball.y > court_radius.y - ball_radius.y) {
		ball.y = court_radius.y - ball_radius.y;
		if (ball_velocity.y > 0.0f) {
			ball_velocity.y = -ball_velocity.y;
		}
	}
	if (ball.y < -court_radius.y + ball_radius.y) {
		ball.y = -court_radius.y + ball_radius.y;
		if (ball_velocity.y < 0.0f) {
			ball_velocity.y = -ball_velocity.y;
		}
	}

	if (ball.x > court_radius.x - ball_radius.x) {
		ball.x = court_radius.x - ball_radius.x;
		if (ball_velocity.x > 0.0f) {
			ball_velocity.x = -ball_velocity.x;
			left_score += 1;
		}
	}
	if (ball.x < -court_radius.x + ball_radius.x) {
		ball.x = -court_radius.x + ball_radius.x;
		if (ball_velocity.x < 0.0f) {
			ball_velocity.x = -ball_velocity.x;
			right_score += 1;
		}
	}
}

void RaidenMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xf2d2b6ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
	const std::vector< glm::u8vec4 > trail_colors = {
		HEX_TO_U8VEC4(0xf2ad9488),
		HEX_TO_U8VEC4(0xf2897288),
		HEX_TO_U8VEC4(0xbacac088),
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//inline helper function for triangle drawing:
	auto draw_diamond = [&vertices](glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color) {
		//draw triangle with all equal size.
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y-radius.x, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y-radius.x, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	auto draw_figher = [&]()
	{
		draw_diamond(bot_fighter, fighter_radius, fg_color);
		glm::vec2 rect1_pos(bot_fighter.x-fighter_radius.x, bot_fighter.y-fighter_radius.y*0.7f);
		glm::vec2 rect2_pos(bot_fighter.x+fighter_radius.x, bot_fighter.y-fighter_radius.y*0.7f);
		draw_rectangle(rect1_pos, glm::vec2(0.05f, 0.05f), fg_color);
		draw_rectangle(rect2_pos, glm::vec2(0.05f, 0.05f), fg_color);
	};

	auto draw_bullets = [&]()
	{
		for (const auto& b : all_bullets)
		{
			draw_rectangle(b.bullet_position, b.bullet_radius, fg_color);
		}
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_diamond(bot_fighter+s, fighter_radius, shadow_color);

	//solid objects:

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//Game objects:
	draw_figher();
	draw_bullets();


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f +padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
