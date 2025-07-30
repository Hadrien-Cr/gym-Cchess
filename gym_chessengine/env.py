import gymnasium as gym
from gymnasium import spaces
import numpy as np
from gym_chessengine.binding import PyChessBoard

class BaseEnv(gym.Env):
    depth: int
    board: PyChessBoard # type: ignore

    def __init__(self, config=None) -> None:
        self.board = PyChessBoard() # type: ignore
        self.depth = config.get("depth", 2) if config else 6
        self.side = config.get("side", None) if config else None

        self.action_space = spaces.Discrete(64 * 64)
        self.observation_space = spaces.Box(low=0, high=1, shape=(12, 8, 8), dtype=np.float64)

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)

        fen = options.get("fen") if options and "fen" in options else \
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        self.board.reset(fen)

        if self.side and self.side != self.board.current_side():
            self.step(self.environment_move())

        return self.board.get_observation(), {}

    def step(self, action):
        obs, reward, done = self.board.step(action)
        terminated = done
        truncated = False  # Add logic if you support truncation
        return obs, reward, terminated, truncated, {}

    def environment_move(self) -> int:
        return self.board.environment_move(self.depth)

    def render(self, mode="human"):
        self.board.print_board()


class ChessSelfPlay(BaseEnv):
    def __init__(self):
        super().__init__({"side": None})

class ChessEngine(BaseEnv):
    def __init__(self, depth = 6, side = "white"):
        super().__init__({"depth": depth, "side": side})
        self.board.init_engine()

    def reset(self, seed=None, options=None):
        super().reset(seed=seed, options=options)
        if not self.side:
            import random
            if random.random() > 0.5:
                self.step(self.environment_move())
            else:
                self.step(self.environment_move())
        return self.board.get_observation(), {}
    
    def step(self, action):
        obs, reward, terminated, truncated, info = super().step(action)
        if not terminated:
            obs, reward, terminated, truncated, info = super().step(self.environment_move())
        return obs, reward, terminated, truncated, info