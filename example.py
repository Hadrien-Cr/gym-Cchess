from gym_chessengine.env_wrapper import BaseEnv


env = BaseEnv()
env.render()
env.reset()
m = env.board.environment_move(2)
print(env.move_to_string(m))
# env.step(env.environment_move())