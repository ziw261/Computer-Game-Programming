#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#define BULLET_LIFETIME 5.0f
#define PLAYER_HEALTH 20.0f
#define ENEMY_HEALTH 10.0f
#define BULLET_DAMAGE 5.0f
#define PLAYER_SPEED 3.0f
#define ENEMY_SPEED 5.0f
#define PLAYER_SHOOT_COOLDOWN 0.1f
#define ENEMY_SHOOT_COOLDOWN 0.05f
#define COURT_RADIUS glm::vec2(7.0f, 5.0f)
#define BULLET_DEVIATION_VALUE 0.1f


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
		glm::vec2 bullet_position = glm::vec2(0);
		glm::vec2 bullet_radius = glm::vec2(0.05f, 0.1f);
		float bullet_speed = 5.0f;

		// If in pool or not
		bool in_bullet_pool = false;
		
		// 1 for player, -1 for enemy
		int shoot_dir = 1;

		// bullet velocity
		glm::vec2 bullet_velocity = glm::vec2(0.0f, shoot_dir * 1.0f);

		// 0 for player, 1 for enemy
		int owner = -1;

		// Bullet lifetime
		float bullet_lifetime = BULLET_LIFETIME;
		Bullet() {};
		Bullet(glm::vec2 pos, float speed) : bullet_position(pos), bullet_speed(speed) {};
	};

	//------ Enemy Struct ------
	struct Enemy
	{
		glm::vec2 enemy_position = glm::vec2(0.0f, COURT_RADIUS.y - 0.5f);
		glm::vec2 enemy_radius = glm::vec2(0.15f, 0.3f);
		float curr_enemy_shoot_cool_down = ENEMY_SHOOT_COOLDOWN;
		Enemy() {};
		Enemy(glm::vec2 pos) : enemy_position(pos) {};
	};

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;


	//------ Raiden Game State -----
	glm::vec2 fighter_radius = glm::vec2(0.2f, 0.4f);
	glm::vec2 bot_fighter = glm::vec2(0.0f, -COURT_RADIUS.y + 0.5f);
	glm::vec4 player_collision_box = glm::vec4(0);
	int curr_status = EventStatus::none;
	float curr_player_shoot_cool_down = PLAYER_SHOOT_COOLDOWN;
	std::vector<Bullet> all_bullets;
	std::deque<int> bullet_pool;
	std::vector<Enemy> all_enemies;
	void execute_event(float elapsed);
	void player_shoot();
	void update_bullet(float elapsed);
	void generate_enemies();
	bool check_collision(const std::vector<glm::vec2>& points, const glm::vec4& box);



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
