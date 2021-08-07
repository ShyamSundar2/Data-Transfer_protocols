all:
	g++ -std=c++17 -pthread SenderSR.cpp -o SenderSR
	g++ -std=c++17 -pthread ReceiverSR.cpp -o ReceiverSR
	g++ -std=c++17 -pthread SenderGBN.cpp -o SenderGBN
	g++ -std=c++17 -pthread ReceiverGBN.cpp -o ReceiverGBN
clean:
	rm -rf SenderSR ReceiverSR SenderGBN ReceiverGBN
