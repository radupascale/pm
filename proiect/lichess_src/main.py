import lichess.api
import berserk
import requests
import threading
import pprint

def start_session():
	with open(".token", "r") as token_file:
		API_TOKEN = token_file.read()
		session = berserk.TokenSession(API_TOKEN)
		return berserk.Client(session=session)
	
def serial_connect():
	pass

class Game(threading.Thread):
	def __init__(self, client, game_id, **kwargs):
		super().__init__(**kwargs)
		self.game_id = game_id
		self.client = client
		self.stream = client.bots.stream_game_state(game_id)
		self.current_state = next(self.stream)

	def run(self):
		for event in self.stream:
			if event['type'] == 'gameState':
				self.handle_state_change(event)
			elif event['type'] == 'chatLine':
				self.handle_chat_line(event)

	def handle_state_change(self, game_state):
		print(game_state)
		pass

	def handle_chat_line(self, chat_line):
		pass

if __name__ == '__main__':
	# TO DO: Connect to arduino serial port

	# Connect to the Lichess server using a bot account and listen for events
	client = start_session()
	stream = client.board.stream_incoming_events()
	for event in stream:
		if event['type'] == 'challenge':
			client.bots.accept_challenge(event['challenge']['id'])
		elif event['type'] == 'gameStart':
			game = Game(client = client, game_id = event['game']['gameId'])
			game.start()


	# TO DO: Send a request to the Arduino to receive the board state

	# TO DO: If this is the first time we are receiving the board, save the position of the pieces

	# TO DO: Print the board state to the console to validate the pieces

	# TO DO: Create an analysis board on Lichess