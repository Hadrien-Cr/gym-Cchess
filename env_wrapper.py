import gym
from gym import spaces
import numpy as np

try:
    from . import myenv_cpp
except (ImportError, SystemError):
    pass

class MyGymEnv(gym.Env):
    def __init__(self):
        self.env = myenv_cpp.MyEnv()
        self.action_space = spaces.Discrete(4096) # A move can be represented as a number from 0 to 4095
        self.observation_space = spaces.Box(low=0, high=1, shape=(12, 8, 8), dtype=np.float64)

    def reset(self, fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "):
        self.env.reset(fen)
        return self.env.get_observation()

    def step(self, action):
        obs, reward, done = self.env.step(action)
        return obs, reward, done, {}

    def environment_move(self):
        return self.env.environment_move()

    def render(self, mode="human"):
        self.env.print_board()