#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * PongMode is a game mode that implements a single-player game of Pong.
 */

enum EventStatus
{
	none = 1 << 0,
	is_up = 1 << 1,
	is_down = 1 << 2,
	is_left = 1 << 3,
	is_right = 1 << 4,
	is_shoot = 1 << 5
};

struct RaidenMode : Mode {
	RaidenMode();
	virtual ~RaidenMode();

	struct Bullet {
		glm::vec3 bullet_position = glm::vec3(0);
		glm::vec2 bullet_radius = glm::vec2(0.05f, 0.1f);
		float bullet_speed = 5.0f;
		
		// 1 for player, -1 for enemy
		int shoot_dir = 1;
		
		// 0 for player, 1 for enemy
		int owner = -1;
		Bullet() {};
		Bullet(glm::vec3 pos, float speed) : bullet_position(pos), bullet_speed(speed) {};
	};

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;


	//------ Raiden Game State -----
	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
	glm::vec2 fighter_radius = glm::vec2(0.2f, 0.4f);
	glm::vec2 bot_fighter = glm::vec2(-court_radius.x + 0.5f, 0.0f);
	glm::vec2 bot_fighter_speed = glm::vec2(3.0f, 3.0f);
	int curr_status = EventStatus::none;
	std::vector<Bullet> all_bullets;
	float player_shoot_cool_down = 0.1f;
	float curr_player_shoot_cool_down = player_shoot_cool_down;
	void execute_event(float elapsed);
	void player_shoot();
	void update_bullet(float elapsed);

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "RaidenMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
