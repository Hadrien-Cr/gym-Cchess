import gymnasium as gym 
from gymnasium import spaces
import numpy as np
from gym_chessengine.env import PyChessBoard

class BaseEnv(gym.Env):
    def __init__(self) -> None:
        self.board = PyChessBoard()
        self.action_space = spaces.Discrete(4096) # A move can be represented as a number from 0 to 4095
        self.observation_space = spaces.Box(low=0, high=1, shape=(12, 8, 8), dtype=np.float64)

    def reset(self, fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ") -> None:
        self.board.reset(fen)
        return self.board.get_observation()

    def move_to_string(self, move: int) ->str:
        start_square = move // 64
        end_square = move % 64
        rank_start = (8 - start_square // 8 )
        file_start = chr(ord("a") + start_square % 8)
        rank_end = (8 - end_square // 8)
        file_end = chr(ord("a") + end_square % 8)
        return(file_start+str(rank_start)+file_end+str(rank_end)) 

    def string_to_move(self, move_string: str) -> int:
        start_square = (ord(move_string[0]) - ord("a")) + (8 - int(move_string[1])) * 8
        end_square = (ord(move_string[2]) - ord("a")) + (8 - int(move_string[3])) * 8
        return start_square * 64 + end_square
        
           
    def step(self, action: int) -> tuple:
        obs, reward, done = self.board.step(action)
        return obs, reward, done, {}

    def environment_move(self) -> int:
        return self.board.environment_move()

    def render(self, mode="human") -> None:
        self.board.print_board()