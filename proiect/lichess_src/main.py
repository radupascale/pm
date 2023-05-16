import berserk
import threading
import chess
from IPython.display import SVG, display
from PIL import Image
import chess.svg
from serialConnect import Arduino
import cairosvg
import cv2
import os
from pprint import pprint

RANKS = 8
FILES = 8
BOARD_STATE_CHANGE = 66

def start_session():
	with open(".token", "r") as token_file:
		API_TOKEN = token_file.read()
		session = berserk.TokenSession(API_TOKEN)
		return berserk.Client(session=session)

class Game(threading.Thread):
	def __init__(self, client, game_id, arduino, **kwargs):
		super().__init__(**kwargs)
		self.game_id = game_id
		self.client = client
		self.stream = client.bots.stream_game_state(game_id)
		self.current_state = next(self.stream)
		self.board = chess.Board()
		self.arduino = arduino

	def run(self):
		"""
		TO DO: Handle moves, chat message, and game state changes such as game end or opponent disconnect
		"""
		for event in self.stream:
			if event['type'] == 'gameState':
				self.handle_state_change(event)
			elif event['type'] == 'chatLine':
				self.handle_chat_line(event)

	def handle_state_change(self, game_state):
		"""
		TO DO: If opponent concedes or the game ends, clean up and close the thread.
		TO DO: If it is the opponents turn, listen for a new gameState, but also check whether or not we have received info from the Arduino
		TO DO: If it our turn, simply check if we have received input from the Arduino

		Args:
			game_state (_type_): _description_
		"""
		
		# Handle the board received from the Arduino, find out what move has been made, and if it is legal, send it to Lichess

	def handle_chat_line(self, chat_line):
		pass

	def handle_move(self, new_board):
		current_board = self.board()

class Displayer(threading.Thread):
	""" Class used for debugging purposes. Displays the current board state in a window.
	Args:
		threading (_type_): _description_
	"""
	def __init__(self, **kwargs):
		super().__init__(**kwargs)
		self.daemon = True
		self.board_path_svg = "images/current_position.svg"
		self.board_path_png = "images/current_position.png"
		self.new_file = "images/new_position.svg"
		self.window_name = "Chessboard"

	def run(self):
		while True:
			if os.path.exists(self.new_file):
				# TO DO: ADD SYNCHRONIZATION HERE
				# Delete the current position file and rename the new position file
				os.remove(self.board_path_svg)
				os.rename(self.new_file, self.board_path_svg)
				self.display_board()
			cv2.waitKey(1)


	def display_board(self):
		cairosvg.svg2png(url=self.board_path_svg, write_to=self.board_path_png)
		
		# Creates a new window, if the window already exists, nothing happens
		cv2.namedWindow(self.window_name, cv2.WINDOW_NORMAL)

		image = cv2.imread(self.board_path_png)
		cv2.imshow(self.window_name, image)

class Reader(threading.Thread):
	def __init__(self, arduino, color,debug = True, **kwargs):
		super().__init__(**kwargs)
		# self.daemon = True
		self.arduino = arduino
		self.board = chess.Board()
		self.debug = debug
		self.new_file = "images/new_position.svg"
		self.color = color # The color of the player

	def create_new_board_file(self):
		with open(self.new_file, "w") as f:
			f.write(chess.svg.board(board = self.board))

	def create_displayer(self):
		self.create_new_board_file()
		displayer = Displayer()
		displayer.start()

	def update_current_board(self, data):
		# The data is received from the eight rank to the first rank and from the first file to the eighth file
		from_fen = self.fen_to_board(self.board.fen())

		# First of all, check if the player castled
		if self.color == chess.WHITE:
			pass

		src_square = None
		dest_square = None

		for i in range(RANKS):
			for j in range(FILES):
				# First determine the source square and the destination square
				# 0 means that there is a piece on the square, 1 means that the square is empty
				if from_fen[i][j] == '' and data[i * RANKS + j] == 0:
					dest_square = chess.square_name(chess.square(j, RANKS - i - 1))
				elif from_fen[i][j] != '' and data[i * RANKS + j] != 0:
					src_square = chess.square_name(chess.square(j, RANKS - i - 1))
				# CASE 1: PIECE MOVED TO AN EMPTY SQUARE

				# CASE 2: PIECE TOOK ANOTHER PIECE

				# CASE 3: CASTLING

				# CASE 4: EN PASSANT

				# CASE 5: PAWN PROMOTION
		move = "0000" # UCI format for null move
		if src_square and dest_square:
			move = src_square + dest_square

		print(move)

		# If debug is set create a new file with the current board state
		try:
			if move != "0000":
				self.board.push_uci(move)
			else:
				# Send this information to the Arduino
				pass
			
			if self.debug:
				self.create_new_board_file()
		except chess.IllegalMoveError:
			# Send this information to the arduino
			print("Illegal move: " + move)
			return

	def fen_to_board(self, fen):
		board = []
		for row in fen.split('/'):
			brow = []
			for c in row:
				if c == ' ':
					break
				elif c in '12345678':
					brow.extend( [''] * int(c))
				elif c == 'p':
					brow.append('bp')
				elif c == 'P':
					brow.append('wp')
				elif c > 'Z':
					brow.append('b'+c.upper())
				else:
					brow.append('w'+c)

			board.append(brow)
		return board

	def run(self):
		if self.debug:
			self.create_displayer()
		self.listen_for_data()

	def listen_for_data(self):
		while True:
			data = self.arduino.get_serial_data()
			# # If the first byte of data has the value 'B', we have received the board state
			if not data:
				continue
			
			data = data.strip()
			pprint(data)

			if data[0] == BOARD_STATE_CHANGE:
				self.update_current_board(data[1:])

if __name__ == '__main__':
	# Connect to Arduino serial port
	arduino = Arduino()

	# Create a thread which listens to the Arduino for input
	reader_thread = Reader(arduino=arduino, color = chess.WHITE)
	reader_thread.start()

	# Connect to the Lichess server using a bot account and listen for events
	# client = start_session()
	# stream = client.board.stream_incoming_events()
	# for event in stream:
	# 	if event['type'] == 'challenge':
	# 		client.bots.accept_challenge(event['challenge']['id'])
	# 	elif event['type'] == 'gameStart':
	# 		game = Game(client = client, game_id = event['game']['gameId'], arduino=arduino)
	# 		game.start()

	# TO DO: Send a request to the Arduino to receive the board state

	# TO DO: If this is the first time we are receiving the board, save the position of the pieces

	# TO DO: Print the board state to the console to validate the pieces

	# TO DO: Create an analysis board on Lichess