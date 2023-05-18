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
from time import sleep
import msvcrt
import subprocess

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
	def __init__(self, position_mutex : threading.Lock, **kwargs):
		super().__init__(**kwargs)
		self.daemon = True
		self.position_mutex = position_mutex
		self.board_path_svg = "images/current_position.svg"
		self.board_path_png = "images/current_position.png"
		self.new_file = "images/new_position.svg"
		self.window_name = "Chessboard"

	def run(self):
		while True:
			if os.path.exists(self.new_file):
				# Delete the current position file and rename the new position file
				self.position_mutex.acquire()
				if os.path.exists(self.board_path_svg):
					os.remove(self.board_path_svg)
				os.rename(self.new_file, self.board_path_svg)
				self.display_board()
				self.position_mutex.release()
			cv2.waitKey(1)


	def display_board(self):
		cairosvg.svg2png(url=self.board_path_svg, write_to=self.board_path_png)
		
		# Creates a new window, if the window already exists, nothing happens
		cv2.namedWindow(self.window_name, cv2.WINDOW_NORMAL)

		image = cv2.imread(self.board_path_png)
		cv2.imshow(self.window_name, image)

class Reader(threading.Thread):
	def __init__(self, arduino, color, board, stop_event, debug = True, **kwargs):
		super().__init__(**kwargs)
		# self.daemon = True
		self.arduino = arduino
		self.board = board
		self.debug = debug
		self.new_file = "images/new_position.svg"
		self.color = color
		self.stop_event = stop_event

	def create_new_board_file(self):
		self.position_mutex.acquire()
		with open(self.new_file, "w") as f:
			f.write(chess.svg.board(board = self.board))
		self.position_mutex.release()

	def create_displayer(self):
		self.position_mutex = threading.Lock() 
		self.create_new_board_file()
		displayer = Displayer(self.position_mutex)
		displayer.start()

	def check_castled(self, data):
		src_square = None
		dest_square = None
		if self.color == chess.WHITE:
			king_square = self.board.king(chess.WHITE)
			# If the king has moved two squares to the left, the player has castled queenside
			if self.board.has_queenside_castling_rights(chess.WHITE) and data[chess.A1:chess.E1 + 1].hex() == '0101000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square - 2)
			# If the king has moved two squares to the right, the player has castled kingside
			elif self.board.has_kingside_castling_rights(chess.WHITE) and data[chess.E1:chess.H1+1].hex() == '01000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square + 2)
		elif self.color == chess.BLACK:
			# Same as above, just change the values to the 8th rank
			king_square = self.board.king(chess.BLACK)
			if self.board.has_queenside_castling_rights(chess.BLACK) and data[chess.A8:chess.E8 + 1].hex() == '0101000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square - 2)
			elif self.board.has_kingside_castling_rights(chess.BLACK) and data[chess.E8:chess.H8+1].hex() == '01000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square + 2)
		return src_square, dest_square

	def update_current_board(self, data):
		piece_map = self.board.piece_map()
		previous_piece_count = len(piece_map)
		current_piece_count = data.count(0)

		# TO DO CASE 1: CHECK IF THE PLAYER CASTLED (values should be not None)
		src_square, dest_square = self.check_castled(data)

		# CASE 2: PIECE MOVED TO AN EMPTY SQUARE
		if not src_square and previous_piece_count == current_piece_count:
			for i in range(RANKS):
				for j in range(FILES):
					if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map:
						src_square = chess.square_name(i * RANKS + j)
					if data[i * RANKS + j] == 0 and i * RANKS + j not in piece_map:
						dest_square = chess.square_name(i * RANKS + j)
		# TO DO CASE 3: PIECE TOOK ANOTHER PIECE
		elif not src_square and previous_piece_count == current_piece_count + 1:
			for i in range(RANKS):
				for j in range(FILES):
					if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map:
						src_square = chess.square_name(i * RANKS + j)
						break
			for square in self.board.attacks(chess.parse_square(src_square)):
				if data[square] == 0 and square in piece_map and piece_map[square].color != piece_map[chess.parse_square(src_square)].color:
					dest_square = chess.square_name(square)
					break

		# TO DO CASE 4: EN PASSANT

		# TO DO CASE 5: PAWN PROMOTION

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

	def print_arduino_data(self, data):
		for i in range(RANKS):
			for j in range(FILES):
				if data[i * RANKS + j] == 1:
					print("-", end=' ')
				else:
					print("X", end=' ')
			print()
		print()

	def validate_position(self, data):
		piece_map = self.board.piece_map()
		for i in range(RANKS):
			for j in range(FILES):
				if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map:
					# TO DO: SEND THIS INFORMATION TO THE ARDUINO
					print("[ERROR]: The board is not in the correct position.")
					return False
		print("The board is in the correct position.")
		return True

	def run(self):
		if self.debug:
			self.create_displayer()
		self.listen_for_data()


	def listen_for_data(self):
		should_validate = True
		while not self.stop_event.is_set():
			data = self.arduino.get_serial_data()
			# If the first byte of data has the value 'B', we have received the board state
			if not data:
				continue

			command = data[0]
			data = data.strip()[1:] # Remove the command byte and the newline character
			self.print_arduino_data(data)

			if command == BOARD_STATE_CHANGE:
				# The data is received from the eight rank to the first rank and from the first file to the eighth file
				# So we reverse it in chunks of 1 byte at a time to not get too confused with the indices
				data = b''.join(reversed([data[x:x+8] for x in range(0, len(data), 8)]))
				# TO DO: If this is the first time we are receiving the board, validate the board state
				if should_validate:
					should_validate = not self.validate_position(data) # This returns True if the position is valid
				else:
					self.update_current_board(data)

def read_kbd_input(inputQueue):
	"""h ttps://stackoverflow.com/questions/5404068/how-to-read-keyboard-input/53344690#53344690

	Args:
		inputQueue (_type_): _description_
	"""
	print('Ready for keyboard input:')
	while (True):
		input_str = input()
		inputQueue.put(input_str)

if __name__ == '__main__':
	# Hardcoded variables to disable and enable the Arduino USB port before connecting to the serial port
	# I have to run the program as administrator for this to work
	devcon = 'C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\10.0.22621.0\\x64\\devcon.exe'
	hwid = "USB\VID_1A86&PID_7523"
	subprocess.run([devcon, 'disable', hwid])
	subprocess.run([devcon, 'enable', hwid])

	board = None
	with open('fen.in', 'r') as f:
		fen = f.read()
		if fen:
			board = chess.Board(fen)
		else:
			board = chess.Board()

	# Connect to Arduino serial port
	arduino = Arduino()

	# Create a thread which listens to the Arduino for input
	stop_event = threading.Event()
	reader_thread = Reader(arduino=arduino, color = chess.WHITE, board = board, stop_event = stop_event)
	reader_thread.start()

	# while (True):
	# 	sleep(100)

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