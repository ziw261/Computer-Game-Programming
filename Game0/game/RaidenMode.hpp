#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>
#include <random>
#include <vector>
#include <deque>

#define BULLET_LIFETIME 5.0f
#define PLAYER_HEALTH 50.0f
#define ENEMY_HEALTH 1.0f
#define BULLET_DAMAGE 5.0f
#define PLAYER_SPEED 5.0f
#define ENEMY_SPEED 5.0f
#define PLAYER_SHOOT_COOLDOWN 0.1f
#define ENEMY_SHOOT_COOLDOWN 1.0f
#define COURT_RADIUS glm::vec2(7.0f, 5.0f)
#define BULLET_DEVIATION_VALUE 0.5f
#define ENEMY_MAX_NUM 15
#define ENEMY_NEED_SPAWN_NUM 5
#define ENEMY_SPAWN_POSSIBILITY 30
#define ROUTE_CHANGE_RATE 4
#define ENEMY_SPAWN_COOL_DOWN 0.4f
#define HEALTH_UI_RADIUS 0.1f


static std::mt19937 mt(std::random_device{}());

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
		Bullet(glm::vec2 pos, float speed, int o, int d) : bullet_position(pos), bullet_speed(speed), owner(o), shoot_dir(d) {};
	};

	//------ Enemy Route ------
	struct EnemyRoute
	{
		//std::mt19937 rng(std::random_device{}());
		int route_length = mt() % 5 + 10;
		std::vector<glm::vec2> route_points;
		EnemyRoute()
		{
			for (int i = 0; i < route_length; i++)
			{
				float x = (mt() % 100) * 0.01f * COURT_RADIUS.x * 2 - COURT_RADIUS.x;
				float y = (mt() % 100) * 0.01f * COURT_RADIUS.y;
				route_points.push_back(glm::vec2(x, y));
			}
		}
	};

	//------ Enemy Struct ------
	struct Enemy
	{
		glm::vec2 enemy_position = glm::vec2(0.0f, COURT_RADIUS.y - 0.5f);
		glm::vec2 enemy_radius = glm::vec2(0.15f, 0.3f);
		glm::vec2 enemy_velocity = glm::vec2(0);
		glm::vec4 enemy_collision_box = glm::vec4(0);
		float curr_enemy_shoot_cool_down = ENEMY_SHOOT_COOLDOWN;
		float enemy_health = ENEMY_HEALTH;
		EnemyRoute enemy_route;
		int route_index = 0;
		bool is_in_pool = false;
		Enemy() {}
		Enemy(glm::vec2 pos, const EnemyRoute& route) : enemy_position(pos), enemy_route(route) {}
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
	int route_change_counter = 0;
	int killed_enemies_num = 0;
	float game_difficulty_mode = 1.0f;
	float curr_player_shoot_cool_down = PLAYER_SHOOT_COOLDOWN;
	float curr_enemy_spawn_cool_down = ENEMY_SPAWN_COOL_DOWN;
	float player_health = PLAYER_HEALTH;
	EnemyRoute curr_route;
	std::vector<Bullet> all_bullets;
	std::deque<int> bullet_pool;
	std::vector<Enemy> all_enemies;
	std::deque<int> enemy_pool;
	void execute_event(float elapsed);
	void player_shoot();
	void enemy_shoot(float elapsed);
	void update_bullet(float elapsed, int r);
	void generate_enemies(float elapsed);
	void update_enemies(float elapsed);
	bool check_collision(const std::vector<glm::vec2>& points, const glm::vec4& box);
	void update_game_data();
	void debug_log();


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
