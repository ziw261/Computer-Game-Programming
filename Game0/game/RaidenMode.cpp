#include "RaidenMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>


RaidenMode::RaidenMode() {
	
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

void RaidenMode::debug_log() {
	std::cout << all_enemies.size() << std::endl;
	std::cout << enemy_pool.size() << std::endl;
}

void RaidenMode::generate_enemies(float elapsed) {

	if ((all_enemies.size() - enemy_pool.size()) >= ENEMY_MAX_NUM + game_difficulty_mode)
		return;
	else if (curr_enemy_spawn_cool_down > 0)
	{
		curr_enemy_spawn_cool_down -= elapsed;
		return;
	}
	else if ((all_enemies.size() - enemy_pool.size()) <= ENEMY_NEED_SPAWN_NUM + game_difficulty_mode
		|| (mt() % 100) < ENEMY_SPAWN_POSSIBILITY + game_difficulty_mode)
	{
		if (route_change_counter == ROUTE_CHANGE_RATE - 1)
		{
			curr_route = EnemyRoute();
		}
		if (!enemy_pool.empty())
		{
			int index = enemy_pool.front();
			enemy_pool.pop_front();
			all_enemies[index].enemy_position = glm::vec2(0.0f, COURT_RADIUS.y - 0.5f);
			all_enemies[index].enemy_velocity = glm::vec2(0);
			all_enemies[index].enemy_collision_box = glm::vec4(0);
			all_enemies[index].curr_enemy_shoot_cool_down = ENEMY_SHOOT_COOLDOWN;
			all_enemies[index].enemy_health = ENEMY_HEALTH + game_difficulty_mode * 0.1f;
			all_enemies[index].enemy_route = curr_route;
			all_enemies[index].route_index = 0;
			all_enemies[index].is_in_pool = false;
		}
		else
		{
			Enemy e(glm::vec2(0.0f, COURT_RADIUS.y - 0.5f), curr_route);
			e.enemy_health = ENEMY_HEALTH + game_difficulty_mode * 0.1f;
			all_enemies.push_back(e);
		}
		route_change_counter = (route_change_counter + 1) % ROUTE_CHANGE_RATE;
		curr_enemy_spawn_cool_down = ENEMY_SPAWN_COOL_DOWN;
	}
}

void RaidenMode::update_game_data() {
	game_difficulty_mode += (killed_enemies_num / 30) * 0.5f;
}

void RaidenMode::update_enemies(float elapsed) {
	for (auto& e : all_enemies) {
		if (e.is_in_pool)
			continue;
		if (e.enemy_position.x >= e.enemy_route.route_points[e.route_index].x - 0.1f
			&& e.enemy_position.x <= e.enemy_route.route_points[e.route_index].x + 0.1f
			&& e.enemy_position.y >= e.enemy_route.route_points[e.route_index].y - 0.1f
			&& e.enemy_position.y <= e.enemy_route.route_points[e.route_index].y + 0.1f)
		{
			e.route_index = (e.route_index + 1) % e.enemy_route.route_length;
		}
		e.enemy_velocity = e.enemy_route.route_points[e.route_index] - e.enemy_position;
		if (e.enemy_velocity != glm::vec2(0))
		{
			e.enemy_position += glm::normalize(e.enemy_velocity) * elapsed * ENEMY_SPEED;

		}
		e.enemy_collision_box = glm::vec4(e.enemy_position.x - e.enemy_radius.x,
			e.enemy_position.x + e.enemy_radius.x,
			e.enemy_position.y - e.enemy_radius.y,
			e.enemy_position.y + e.enemy_radius.y + 0.05f * 0.75f);
	}
}

bool RaidenMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	switch (evt.key.keysym.sym) {
	case SDLK_UP:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_up : curr_status & ~EventStatus::is_up;
		break;
	case SDLK_w:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status | EventStatus::is_up : curr_status & ~EventStatus::is_up;
		break;
	case SDLK_DOWN:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_down : curr_status & ~EventStatus::is_down;
		break;
	case SDLK_s:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status | EventStatus::is_down : curr_status & ~EventStatus::is_down;
		break;
	case SDLK_LEFT:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_left : curr_status & ~EventStatus::is_left;
		break;
	case SDLK_a:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status | EventStatus::is_left : curr_status & ~EventStatus::is_left;
		break;
	case SDLK_RIGHT:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status|EventStatus::is_right : curr_status & ~EventStatus::is_right;
		break;
	case SDLK_d:
		curr_status = evt.type == SDL_KEYDOWN ? curr_status | EventStatus::is_right : curr_status & ~EventStatus::is_right;
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
		movement.y -= 1.0f;
	if (curr_status & EventStatus::is_up)
		movement.y += 1.0f;
	if (curr_status & EventStatus::is_left)
		movement.x -= 1.0f;
	if (curr_status & EventStatus::is_right)
		movement.x += 1.0f;
	if (curr_status & EventStatus::is_shoot) {
		if (curr_player_shoot_cool_down > 0)
		{
			curr_player_shoot_cool_down -= elapsed;
		}
		else
		{
			player_shoot();
			curr_player_shoot_cool_down = PLAYER_SHOOT_COOLDOWN;
		}
	}

	if (movement != glm::vec2(0)) {
		bot_fighter += glm::normalize(movement) * elapsed * PLAYER_SPEED;
	}

	player_collision_box = glm::vec4(bot_fighter.x - fighter_radius.x,
		bot_fighter.x + fighter_radius.x,
		bot_fighter.y - fighter_radius.y - 0.05f,
		bot_fighter.y + fighter_radius.y);
}

void RaidenMode::enemy_shoot(float elapsed) {
	for (auto& e : all_enemies)
	{
		if (e.curr_enemy_shoot_cool_down > 0)
		{
			e.curr_enemy_shoot_cool_down -= elapsed;
			continue;
		}
		else if (e.is_in_pool)
		{
			continue;
		}
		else
		{
			if (!bullet_pool.empty())
			{
				int index = bullet_pool.front();
				bullet_pool.pop_front();
				all_bullets[index].bullet_position = glm::vec2(e.enemy_position.x, e.enemy_position.y - e.enemy_radius.y - 0.05f);
				all_bullets[index].shoot_dir = -1;
				all_bullets[index].owner = 1;
				all_bullets[index].bullet_velocity = glm::vec2(0.0f, all_bullets[index].shoot_dir * 1.0f);
				all_bullets[index].bullet_lifetime = BULLET_LIFETIME;
				all_bullets[index].in_bullet_pool = false;
			}

			Bullet b(glm::vec2(e.enemy_position.x, e.enemy_position.y - e.enemy_radius.y - 0.05f), 7.0f, 1, -1);
			all_bullets.push_back(b);
			e.curr_enemy_shoot_cool_down = ENEMY_SHOOT_COOLDOWN;
		}
	}
}

void RaidenMode::player_shoot() {
	if (!bullet_pool.empty())
	{
		int index = bullet_pool.front();
		bullet_pool.pop_front();
		all_bullets[index].bullet_position = glm::vec2(bot_fighter.x, bot_fighter.y + fighter_radius.y + 0.05f);
		all_bullets[index].shoot_dir = 1;
		all_bullets[index].owner = 0;
		all_bullets[index].bullet_velocity = glm::vec2(0.0f, all_bullets[index].shoot_dir * 1.0f);
		all_bullets[index].bullet_lifetime = BULLET_LIFETIME;
		all_bullets[index].in_bullet_pool = false;
	}
	else
	{
		Bullet b(glm::vec2(bot_fighter.x, bot_fighter.y + fighter_radius.y + 0.05f), 7.0f, 0, 1);
		all_bullets.push_back(b);
	}
}

bool RaidenMode::check_collision(const std::vector<glm::vec2>& points, const glm::vec4& box) {
	for (int i = 0; i < points.size(); i++)
	{
		if (points[i].x >= box[0] && points[i].x <= box[1]
			&& points[i].y >= box[2] && points[i].y <= box[3])
		{
			return true;
		}
	}
	return false;
}

void RaidenMode::update_bullet(float elapsed, int random) {
	for (int i=0; i<all_bullets.size(); i++)
	{
		Bullet& b = all_bullets[i];
		if (b.in_bullet_pool)
			continue;

		if (b.bullet_lifetime <= 0)
		{
			bullet_pool.push_back(i);
			b.in_bullet_pool = true;
		}
		else
		{
			b.bullet_position += b.bullet_speed * b.bullet_velocity * elapsed;
			if (b.owner == 0)
			{
				if (b.bullet_position.y > COURT_RADIUS.y - b.bullet_radius.y)
				{
					b.bullet_position.y = COURT_RADIUS.y - b.bullet_radius.y;
					if (b.bullet_velocity.y > 0.0f)
					{
						b.bullet_velocity.y = -b.bullet_velocity.y;
						b.bullet_velocity.x -= BULLET_DEVIATION_VALUE * random;
					}
				}
				if (b.bullet_position.y < -COURT_RADIUS.y + b.bullet_radius.y)
				{
					b.bullet_position.y = -COURT_RADIUS.y + b.bullet_radius.y;
					if (b.bullet_velocity.y < 0.0f)
					{
						b.bullet_velocity.y = -b.bullet_velocity.y;
						b.bullet_velocity.x += -BULLET_DEVIATION_VALUE * random;
					}
				}

				if (b.bullet_position.x > COURT_RADIUS.x - b.bullet_radius.x)
				{
					b.bullet_position.x = COURT_RADIUS.x - b.bullet_radius.x;
					if (b.bullet_velocity.x > 0.0f)
					{
						b.bullet_velocity.x = -b.bullet_velocity.x;
						b.bullet_velocity.y -= BULLET_DEVIATION_VALUE * random;
					}
				}
				if (b.bullet_position.x < -COURT_RADIUS.x + b.bullet_radius.x)
				{
					b.bullet_position.x = -COURT_RADIUS.x + b.bullet_radius.x;
					if (b.bullet_velocity.x < 0.0f)
					{
						b.bullet_velocity.x = -b.bullet_velocity.x;
						b.bullet_velocity.y += -BULLET_DEVIATION_VALUE * random;
					}
				}
			}

			// ------ Check Collision ------ //
			{
				glm::vec2 p1 = glm::vec2(b.bullet_position.x - b.bullet_radius.x, b.bullet_position.y);
				glm::vec2 p2 = glm::vec2(b.bullet_position.x + b.bullet_radius.x, b.bullet_position.y);
				glm::vec2 p3 = glm::vec2(b.bullet_position.x, b.bullet_position.y + b.bullet_radius.y);
				glm::vec2 p4 = glm::vec2(b.bullet_position.x, b.bullet_position.y - b.bullet_radius.y);

				std::vector<glm::vec2> points = { p1,p2,p3,p4 };
				if (check_collision(points, player_collision_box))
				{
					bullet_pool.push_back(i);
					b.in_bullet_pool = true;
					//std::cout << "Player hit" << std::endl;
					player_health -= BULLET_DAMAGE;
					//if (player_health <= 0)

				}

				if (b.owner == 0)
				{
					for (int index = 0; index < all_enemies.size(); index++)
					{
						if (!all_enemies[index].is_in_pool && check_collision(points, all_enemies[index].enemy_collision_box))
						{
							//std::cout << "enemy hit" << std::endl;
							bullet_pool.push_back(i);
							b.in_bullet_pool = true;
							all_enemies[index].enemy_health -= BULLET_DAMAGE;
							if (all_enemies[index].enemy_health <= 0)
							{
								enemy_pool.push_back(index);
								all_enemies[index].is_in_pool = true;
								killed_enemies_num++;
								//std::cout << "enemy dead" << std::endl;
							}
						}
					}
				}
			}

			b.bullet_lifetime -= elapsed;
		}
	}
}

void RaidenMode::update(float elapsed) {

	//static std::mt19937 mt(std::random_device{}()); //mersenne twister pseudo-random number generator

	//debug_log();

	if (player_health <= 0){
		generate_enemies(elapsed);
		update_enemies(elapsed);
		return;
	}

	// Execute Keyboard event
	execute_event(elapsed);
	
	// Update bullet
	int r = mt() % 2 == 0 ? 1 : -1;
	update_bullet(elapsed, r);

	//clamp fighters to court:
	bot_fighter.x = std::max(bot_fighter.x, -COURT_RADIUS.x + fighter_radius.x);
	bot_fighter.x = std::min(bot_fighter.x, COURT_RADIUS.x - fighter_radius.x);
	bot_fighter.y = std::max(bot_fighter.y, -COURT_RADIUS.y + fighter_radius.y);
	bot_fighter.y = std::min(bot_fighter.y, COURT_RADIUS.y - fighter_radius.y);
	
	// Enemy
	generate_enemies(elapsed);
	update_enemies(elapsed);
	enemy_shoot(elapsed);

	update_game_data();
}

void RaidenMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x191716ff);
	const glm::u8vec4 player_color = HEX_TO_U8VEC4(0xf2d2b6ff);
	const glm::u8vec4 enemy_color = HEX_TO_U8VEC4(0xbb4430ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
	// Green, good
	const glm::u8vec4 health_color_g = HEX_TO_U8VEC4(0x16c1c4ff);
	// Yello, soso
	const glm::u8vec4 health_color_y = HEX_TO_U8VEC4(0xfdde9aff);
	// Red, not good
	const glm::u8vec4 health_color_r = HEX_TO_U8VEC4(0xda3d19ff);
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

	//inline helper function for diamod shape drawing:
	auto draw_diamond = [&vertices](glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color) {
		//draw triangle with all equal size.
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y-radius.x, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x, center.y-radius.x, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	auto draw_figher = [&](const glm::vec2& pos, const glm::vec2& radius, const glm::u8vec4& color, const int is_player)
	{
		draw_diamond(pos, radius, color);
		glm::vec2 rect1_pos(pos.x-radius.x, pos.y-radius.y*0.7f * is_player);
		glm::vec2 rect2_pos(pos.x+radius.x, pos.y-radius.y*0.7f * is_player);
		float back_wings_length = is_player == 1 ? 0.05f : 0.05f * 0.75f;
		draw_rectangle(rect1_pos, glm::vec2(back_wings_length, back_wings_length), color);
		draw_rectangle(rect2_pos, glm::vec2(back_wings_length, back_wings_length), color);
	};

	auto draw_bullets = [&]()
	{
		for (const auto& b : all_bullets)
		{
			if (!b.in_bullet_pool)
			{
				const glm::u8vec4& bullet_color = b.owner == 0 ? player_color : enemy_color;
				draw_rectangle(b.bullet_position, b.bullet_radius, bullet_color);
			}
		}
	};

	auto draw_enemies = [&]()
	{
		for (const auto& e : all_enemies)
		{
			if (!e.is_in_pool)
				draw_figher(e.enemy_position, e.enemy_radius, enemy_color, -1);
		}
	};

	auto draw_health = [&]()
	{
		if (player_health <= 0)
			return;

		float health_propertion = player_health / PLAYER_HEALTH;
		float difference = (1.0f - health_propertion) * COURT_RADIUS.x;
		glm::vec2 health_bar_pos = glm::vec2(0 - difference, -COURT_RADIUS.y-1.5f*HEALTH_UI_RADIUS-padding/2.0f); 
		glm::vec2 health_bar_radius = glm::vec2((COURT_RADIUS.x + padding) * health_propertion, padding + 1.5f*HEALTH_UI_RADIUS);

		glm::u8vec4 health_color = health_propertion > 0.6f ? health_color_g : 
									health_propertion > 0.4f ? health_color_y:
									health_color_r;

		draw_rectangle(health_bar_pos, health_bar_radius, health_color);
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);


	//Game objects:
	if (player_health > 0){
		draw_figher(bot_fighter, fighter_radius, player_color, 1);
		draw_diamond(bot_fighter+s, fighter_radius, shadow_color);
		draw_bullets();
	}
	draw_enemies();
	draw_health();


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-COURT_RADIUS.x - 2.0f * wall_radius - padding,
		-COURT_RADIUS.y - 2.0f * wall_radius - padding - 3.0f * HEALTH_UI_RADIUS
	);
	glm::vec2 scene_max = glm::vec2(
		COURT_RADIUS.x + 2.0f * wall_radius + padding,
		COURT_RADIUS.y + 2.0f * wall_radius + padding
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
