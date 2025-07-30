import gym_chessengine
import gymnasium as gym
from gym_chessengine import string_to_move

env = gym.make("ChessEngine-v0", depth = 2, side = "black")
env.reset()
env.render()
env.step(string_to_move("d7d5"))
env.render()