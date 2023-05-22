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
import sys
import logging

RANKS = 8
FILES = 8
BOARD_STATE_CHANGE = 66
BOARD_STATE_INFO = 77

class Reader(threading.Thread):
	def __init__(self, arduino : Arduino, board_lock : threading.Lock, move_event : threading.Event, client, client_2, debug = True,**kwargs):
		super().__init__(**kwargs)
		# self.daemon = True
		self.arduino = arduino
		self.board = chess.Board()
		self.color = chess.WHITE # TODO: CHANGE THIS BACK TO NONE
		self.debug = debug
		self.new_file = "images/new_position.svg"
		self.move_event = move_event
		self.board_lock = board_lock
		self.is_otb = True # TODO: CHANGE THIS BACK TO FALSE
		self.main_bot = client
		self.bot_friend = client_2 # Used for OTB

	def new_game_setup(self, gameStartEvent):
		self.board = chess.Board(fen = gameStartEvent['game']['fen'])
		if self.is_otb:
			self.color = chess.WHITE
		else:
			self.color = gameStartEvent['game']['color'] == 'white'

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
		# There is a glitch here if the player tries to castle with no rights. The script will think that the player moved the rook.
		# If I try to fix it by not checking for castling rights, the script will think that the player tries to castle
		# if the rook moves near the king and the king is moved two squares to the left or right. 
		if self.board.turn == chess.WHITE:
			king_square = self.board.king(chess.WHITE)
			# If the king has moved two squares to the left, the player has castled queenside
			if self.board.has_castling_rights(chess.WHITE) and data[chess.A1:chess.E1 + 1].hex() == '0101000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square - 2)
			# If the king has moved two squares to the right, the player has castled kingside
			elif self.board.has_castling_rights(chess.WHITE) and data[chess.E1:chess.H1+1].hex() == '01000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square + 2)
		else:
			# Same as above, just change the values to the 8th rank
			king_square = self.board.king(chess.BLACK)
			if self.board.has_castling_rights(chess.BLACK) and data[chess.A8:chess.E8 + 1].hex() == '0101000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square - 2)
			elif self.board.has_castling_rights(chess.BLACK) and data[chess.E8:chess.H8+1].hex() == '01000001':
				src_square = chess.square_name(king_square)
				dest_square = chess.square_name(king_square + 2)
		return src_square, dest_square

	def check_moved_to_empty_square(self, data, piece_map):
		"""
		The MOVED logic is as follows: The player moves the piece to the desired square and sends the request to the script.
		The script then parses the move and checks if it is legal.
		Args:
			data (_type_): _description_
			piece_map (_type_): _description_

		Returns:
			_type_: _description_
		"""
		src_square = None
		dest_square = None
		for i in range(RANKS):
			for j in range(FILES):
				if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map:
					src_square = chess.square_name(i * RANKS + j)
				if data[i * RANKS + j] == 0 and i * RANKS + j not in piece_map:
					dest_square = chess.square_name(i * RANKS + j)

		return src_square, dest_square


	def check_takes(self, data, piece_map):
		"""
		The TAKES logic is as follows: The player lifts BOTH pieces from the board and sends the request to the script
		The script then parses the move and checks if it is legal. If it is, then the player can put the piece that took
		on the board. If it is not, then the player has to retry sending the move. This is done because the
		sensors sometimes give false positives.
		Args:
			data (_type_): _description_
			piece_map (_type_): _description_

		Returns:
			_type_: _description_
		"""
		src_square = None
		dest_square = None
		for i in range(RANKS):
			for j in range(FILES):
				if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map and self.board.piece_at(i * RANKS + j).color == self.board.turn:
					src_square = chess.square_name(i * RANKS + j)
				if data[i * RANKS + j] == 1 and i * RANKS + j in piece_map and self.board.piece_at(i * RANKS + j).color != self.board.turn:
					dest_square = chess.square_name(i * RANKS + j)
		return src_square, dest_square

	def check_en_passant(self, data, piece_map):
		"""
		The en passant logic is as follows: The player moves the pawn that is taking the other pawn to en passant square
		and then removes the pawn that is being taken from the board. The script then parses the move and checks if it is legal.
		Args:
			data (_type_): _description_
			piece_map (_type_): _description_
		"""
		src_square = None
		dest_square = None
		if self.board.has_legal_en_passant():
			en_passant_square = self.board.ep_square
			
			if data[en_passant_square] == 0:
				dest_square = chess.square_name(en_passant_square)

			if self.board.turn == chess.WHITE:
				# Check the fifth rank for source square
				rank = 4
				for file in range(FILES):
					if data[rank * RANKS + file] == 1:
						piece = self.board.piece_at(rank * RANKS + file)
						if piece is not None and piece.symbol() == 'P':
							src_square = chess.square_name(rank * RANKS + file)
			else:
				rank = 3
				for file in range(FILES):
					if data[rank * RANKS + file] == 1:
						piece = self.board.piece_at(rank * RANKS + file)
						if piece is not None and piece.symbol() == 'p':
							src_square = chess.square_name(rank * RANKS + file)
		return src_square, dest_square

	def check_promotion(self, move : str):
		"""
		Checks to see if the piece has been promoted. If it has add 'q' to the end of the move.

		Args:
			move (_type_): String tuple containing the source square and the destination square
		"""
		src_square, dest_square = move
		final_move = src_square + dest_square
		if chess.Move.from_uci(final_move + 'q') in self.board.legal_moves:
			final_move += 'q'
		return final_move

	def update_current_board(self, data):
		piece_map = self.board.piece_map()
		previous_piece_count = len(piece_map)
		current_piece_count = data.count(0)

		# CASE 1: CHECK IF THE PLAYER CASTLED (values should be not None)
		src_square, dest_square = self.check_castled(data)

		# CASE 2: PIECE MOVED TO AN EMPTY SQUARE
		if not src_square and previous_piece_count == current_piece_count:
			src_square, dest_square = self.check_moved_to_empty_square(data, piece_map)
		# CASE 3: PIECE TOOK ANOTHER PIECE
		elif not src_square and previous_piece_count == current_piece_count + 2:
			src_square, dest_square = self.check_takes(data, piece_map)
		# CASE 4: EN PASSANT
		elif not src_square and previous_piece_count == current_piece_count + 1:
			src_square, dest_square = self.check_en_passant(data, piece_map)

		move = "0000" # UCI format for null move 
		if src_square and dest_square:
			move = self.check_promotion((src_square, dest_square))

		# If debug is set create a new file with the current board state
		try:
			if move != "0000":
				self.board_lock.acquire()
				self.board.push_uci(move)
				self.board_lock.release()
				logging.info(f'Played move: {move}')
				self.move_event.set()
				# Make sure to change the color when playing OTB
				if self.is_otb:
					self.color = not self.color
			else:
				# TODO: Send this information to the Arduino
				logging.warning("Null move")
			
			if self.debug:
				self.create_new_board_file()
		except chess.IllegalMoveError:
			# TODO: Send this information to the arduino
			logging.warning("Illegal move: " + move)
			self.reader.board_lock.release()
			return
	
	def update_board_with_opponent_move(self, move):
		"""
		Update the board and send the new configuration to the Arduino.

		Args:
			move (_type_): _description_
		"""
		self.reader.board_lock.acquire()
		self.reader.board.push_uci(move)
		self.reader.board_lock.release()
		self.send_board_to_arduino()

	def send_board_to_arduino(self):
		"""
		Send the current board configuration to the Arduino
		"""
		# This generates the configuration of the current board as a bytearray by converting every piece to its
		# ASCII value
		board_matrix = bytearray([ord(self.board.piece_map()[x].symbol()) if x in list(self.board.piece_map()) else 1 for x in chess.SQUARES])
		# Before sending the data to the arduino, we add the header byte and a newline at the end
		board_matrix = bytes(BOARD_STATE_CHANGE.to_bytes() + board_matrix + ord('\n').to_bytes())
		# Finally, send the data to the ARDUINO
		self.arduino.send_serial_data(board_matrix)


	def print_arduino_board(self, data):
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
					logging.error("The board is not in the correct position.")
					return False
		logging.info("The board is in the correct position.")
		return True

	def run(self):
		if self.debug:
			self.create_displayer()
		self.listen_for_data()

	def listen_for_data(self):
		while True:
			data = self.arduino.get_serial_data()
			# If the first byte of data has the value 'B', we have received the board state
			if not data:
				continue

			command = data[0]
			data = data.strip()[1:] # Remove the command byte and the newline character

			if command == BOARD_STATE_CHANGE and self.color == self.board.turn:
				self.print_arduino_board(data)
				# The data is received from the eight rank to the first rank and from the first file to the eighth file
				# So we reverse it in chunks of 1 byte at a time to not get too confused with the indices
				data = b''.join(reversed([data[x:x+8] for x in range(0, len(data), 8)]))
				# Only make a move if its your turn
				self.update_current_board(data)
				self.send_board_to_arduino() # TODO: remove this
			else:
				print(data)
			# TODO: Send an OTB request from the bot_friend to the main bot if the arduino requests this
			# if command == START_OTB
				# self.is_otb = True
				# client_2.challenges.create(client.account.get()['id'], rated=False)

def start_session(token_path):
	with open(token_path, "r") as token_file:
		API_TOKEN = token_file.read()
		session = berserk.TokenSession(API_TOKEN)
		return berserk.Client(session=session)

class Game(threading.Thread):
	def __init__(self, client, gameID, arduino, reader : Reader, color, **kwargs):
		super().__init__(**kwargs)
		self.game_id = gameID
		self.client = client
		self.stream = client.bots.stream_game_state(self.game_id)
		self.current_state = next(self.stream)
		self.arduino = arduino
		self.reader = reader
		self.color = color

	def spawn_move_thread(self):
		"""
		Createa a new daemon thread which sends the move to Lichess.
		"""
		moveThread = threading.Thread(target=self.make_move)
		moveThread.daemon = True
		moveThread.start()

	def make_move(self):
		"""
		Wait for the Reader thread to signal that a move has been made and then send it to Lichess.
		"""
		self.reader.move_event.wait()
		self.client.bots.make_move(self.game_id, self.reader.board.peek().uci())
		self.reader.move_event.clear()
		self.reader.send_board_to_arduino()

	def run(self):
		"""
		TO DO: Handle moves, chat message, and game state changes such as game end or opponent disconnect
		"""
		stop = False
		# If it is our turn, play a move before listening for events
		if self.reader.board.turn == self.color:
			self.spawn_move_thread()

		for event in self.stream:
			print("Received game event :{event}\n".format(event=event))
			if event['type'] == 'gameState':
				stop = self.handle_state_change(event)
			elif event['type'] == 'chatLine':
				self.handle_chat_line(event)
			elif event['type'] == 'opponentGone':
				self.handle_opponent_gone(event)
			if stop:
				break

	def handle_state_change(self, game_state):
		"""
		TO DO: If opponent concedes or the game ends, clean up and close the thread.
		TO DO: If it is the opponents turn, listen for a new gameState, but also check whether or not we have received info from the Arduino
		TO DO: If it our turn, simply check if we have received input from the Arduino
		Args:
			game_state (_type_): _description_
		"""
		if game_state['status'] == 'started' and self.reader.is_otb:
			# If we are the client which played a move last turn, don't make a move.
			if self.color is not self.reader.board.turn:
				return False
			self.spawn_move_thread()
		elif game_state['status'] == 'started' and not self.reader.is_otb:
			last_move = game_state['moves'].split()[-1]
			try:
				# If the last move is the same as the last move we made, don't make a move.
				if last_move == self.reader.board.peek().uci():
					return False
			except IndexError:
				# No move has been added to the board yet
				pass
			self.reader.update_board_with_opponent_move()
			self.spawn_move_thread()
		elif game_state['status'] == 'aborted' or game_state['status'] == 'mate' or game_state['status'] == 'resign' or game_state['status'] == 'outoftime' or game_state['status'] == 'stalemate':
			# Maybe add these in a list and check if the status is in the list
			return True
		return False

		
		# Handle the board received from the Arduino, find out what move has been made, and if it is legal, send it to Lichess

	def handle_chat_line(self, chat_line):
		pass

	def handle_opponent_gone(self, opponent_gone):
		pass

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

if __name__ == '__main__':
	# Hardcoded variables to disable and enable the Arduino USB port before connecting to the serial port
	# I have to run the program as administrator for this to work
	# devcon = 'C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\10.0.22621.0\\x64\\devcon.exe'
	devcon = '.\\utils\\devcon.exe'
	hwid = "USB\VID_1A86&PID_7523"
	subprocess.run([devcon, 'disable', hwid])
	subprocess.run([devcon, 'enable', hwid])

	# Used for logging
	logging.basicConfig(
		format='%(asctime)s %(levelname)-8s %(message)s',
		level=logging.INFO,
		datefmt='%Y-%m-%d %H:%M:%S')

	# Connect to Arduino serial port
	arduino = Arduino()

	# Connect to the Lichess server using a bot account and listen for events
	# TO DO: Start reader as daemon. All clients use the same reader.
	client = start_session("./utils/.token")
	client_2 = start_session("./utils/.token2")

	board_lock = threading.Lock()
	move_event = threading.Event()
	reader = Reader(arduino = arduino, board_lock = board_lock, move_event = move_event, debug = False, client = client, client_2 = client_2)
	reader.daemon = True
	reader.start()

	while True:
		sleep(1000)

	# stream = client.board.stream_incoming_events()
	# for event in stream:
	# 	print("Received event: {event}".format(event=event))
	# 	if event['type'] == 'challenge':
	# 		client.bots.accept_challenge(event['challenge']['id'])
	# 	elif event['type'] == 'gameStart' and not reader.is_otb:
	# 		reader.new_game_setup(gameStartEvent = event)
	# 		game = Game(client = client, arduino=arduino, gameID = event['game']['gameId'], reader = reader, color = reader.color)
	# 		game.start()
	# 		threading.Thread.join(game)
	# 	elif event['type'] == 'gameStart' and reader.is_otb:
	# 		reader.new_game_setup(gameStartEvent = event)
	# 		bot_color = event['game']['color'] == 'white'
	# 		game_bot = Game(client = client, arduino=arduino, gameID = event['game']['gameId'], reader = reader, color = bot_color)
	# 		game_bot_friend = Game(client = client_2, arduino=arduino, gameID = event['game']['gameId'], reader = reader, color = not bot_color)
	# 		l = [game_bot, game_bot_friend]
	# 		for bot in l:
	# 			bot.start()
	# 		for bot in l:
	# 			threading.Thread.join(bot)
