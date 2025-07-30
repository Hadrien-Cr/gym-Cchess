from gymnasium.envs.registration import register

register(
    id="ChessSelfPlay-v0",
    entry_point="gym_chessengine.env:ChessSelfPlay", 
    max_episode_steps=200,
)
register(
    id="ChessEngine-v0",
    entry_point="gym_chessengine.env:ChessEngine", 
    max_episode_steps=200,
)

def move_to_string(move: int) -> str:
    start_square = move // 64
    end_square = move % 64
    rank_start = 8 - (start_square // 8)
    file_start = chr(ord("a") + start_square % 8)
    rank_end = 8 - (end_square // 8)
    file_end = chr(ord("a") + end_square % 8)
    return f"{file_start}{rank_start}{file_end}{rank_end}"

def string_to_move(move_string: str) -> int:
    start = (ord(move_string[0]) - ord("a")) + (8 - int(move_string[1])) * 8
    end = (ord(move_string[2]) - ord("a")) + (8 - int(move_string[3])) * 8
    return start * 64 + end
