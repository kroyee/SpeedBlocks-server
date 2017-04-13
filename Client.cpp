#include "Client.h"
#include "Connections.h"

void Client::authUser() {
	std::cout << "Authing user..." << std::endl;
	sf::Http::Request request("/speedblocks/smftestauth.php", sf::Http::Request::Post);

    sf::String stream = "name=" + name + "&pass=" + authpass;
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        if (response.getBody() != "Failed") {
        	authpass = response.getBody();
        	short c = authpass.find('%');
        	name = authpass.substring(c+1, 100);
        	authpass = authpass.substring(0, c);
        	std::cout << "Auth successfull " << std::endl;
        	authresult=1;
        }
        else {
        	std::cout << "Auth failed" << std::endl;
        	authresult=2;
        }
    }
    else {
        std::cout << "Request failed" << std::endl;
        authresult=2; // Set this to 3 to revert to resending request if the request failed
    }
}

void Client::sendData() {
	sf::Http::Request request("/speedblocks/add.php", sf::Http::Request::Post);

	sf::String tmp = to_string(s_avgBpm);
	unsigned short c = tmp.find('.');
	if (c != sf::String::InvalidPos)
		tmp = tmp.substring(0,c+3);

    sf::String stream = "id="+to_string(id)+"&maxcombo="+to_string(s_maxCombo)+"&maxbpm="+to_string(s_maxBpm);
    stream += "&gamesplayed="+to_string(s_gamesPlayed)+"&avgbpm="+tmp;
    stream += "&gameswon="+to_string(s_gamesWon)+"&rank="+to_string(s_rank)+"&points="+to_string(s_points);
    stream += "&heropoints="+to_string(s_heropoints)+"&totalbpm="+to_string(s_totalBpm);
    stream += "&totalgamesplayed="+to_string(s_totalGames)+"&herorank="+to_string(s_herorank);
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        	std::cout << "Client " << (int)id << ": " << response.getBody() << std::endl;
        	sdataSet=true;
    }
    else {
        std::cout << "request failed" << std::endl;
        sdataSetFailed=true;
    }
}

void Client::getData() {
	std::cout << "Getting data for " << (int)id << std::endl;
	sf::Http::Request request("/speedblocks/get.php", sf::Http::Request::Post);

    sf::String stream = "id=" + to_string(id);
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
    	if (response.getBody().substr(0,5) == "Empty") {
    		std::cout << "No stats found for " << (int)id << std::endl;
    		s_maxCombo = 0; s_maxBpm = 0; s_avgBpm = 0; s_gamesPlayed = 0; s_gamesWon = 0; s_rank = 25; s_points = 0;
    		s_heropoints=0; s_herorank=0; s_totalGames=0; s_totalBpm=0;
    		sdataInit=true;
    		sdataSet=true;
    	}
    	else {
	    	std::string data = response.getBody();
	    	std::cout << data << std::endl;
	    	short c2;
	    	short c = data.find('%');
	    	s_avgBpm = getDataFloat(0,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_gamesPlayed = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_gamesWon = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_heropoints = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_herorank = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_maxBpm = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_maxCombo = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_points = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_rank = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_totalBpm = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_totalGames = getDataInt(c2,c,data);
	    	std::cout << "Data retrieved for " << (int)id << std::endl;
	    	sdataInit=true;
	    	sdataSet=true;
	    }
    }
    else {
        std::cout << "request failed" << std::endl;
        sdataSetFailed = true;
    }
}

int Client::getDataInt(short c2, short c, std::string& data) {
	if (c > c2)
		return stoi(data.substr(c2, c));
	else
		return 0;
}

float Client::getDataFloat(short c2, short c, std::string& data) {
	if (c > c2)
		return stof(data.substr(c2, c));
	else
		return 0;
}

void Client::copy(Client& client) {
	s_maxCombo=client.s_maxCombo; s_maxBpm=client.s_maxBpm; s_rank=client.s_rank;
	s_points=client.s_points; s_heropoints=client.s_heropoints; s_herorank=client.s_herorank;
	s_avgBpm=client.s_avgBpm; s_totalBpm=client.s_totalBpm;
	s_gamesPlayed=client.s_gamesPlayed; s_gamesWon=client.s_gamesWon; s_totalGames=client.s_totalGames;
	sdataInit=true;
	away=false;
	conn=client.conn;
}

Client::Client(const Client& client) {
	id = client.id;  room=nullptr; sdataSet=false; sdataSetFailed=false; sdataInit=false; sdataPut=false;
	
	maxCombo = client.maxCombo; position = client.position;
	linesSent = client.linesSent; linesReceived = client.linesReceived; linesBlocked = client.linesBlocked;
	bpm = client.bpm; spm = client.spm;

	s_maxCombo = client.s_maxCombo; s_maxBpm = client.s_maxBpm; s_rank = client.s_rank;
	s_points = client.s_points; s_heropoints = client.s_heropoints; s_herorank = client.s_herorank;
	s_avgBpm = client.s_avgBpm; s_gamesPlayed = client.s_gamesPlayed; s_gamesWon = client.s_gamesWon;
	s_totalGames = client.s_totalGames; s_totalBpm = client.s_totalBpm;

	conn=client.conn;
}

void Client::checkIfStatsSet() {
	if (sdataSet)
		if (thread.joinable()) {
			thread.join();
			sdataSet=false;
		}
	if (sdataSetFailed)
		if (thread.joinable()) {
			thread.join();
			sdataSetFailed=false;
			thread = std::thread(&Client::getData, this);
		}
}

void Client::checkIfAuth() {
	if (authresult) {
		if (thread.joinable()) {
			thread.join();
			sf::Uint8 packetid = 9;
			if (authresult==1) { //9-Packet nr2
				conn->packet.clear();
				conn->packet << packetid;
				packetid=1;
				std::cout << "Client: " << id << " -> ";
				id = stoi(authpass.toAnsiString());
				std::cout << id << std::endl;
				conn->packet << packetid << name << id;
				conn->send(*this);
				guest=false;
				bool copyfound=false;
				for (auto it = conn->uploadData.begin(); it != conn->uploadData.end(); it++)
					if (it->id == id && !it->sdataPut) {
						std::cout << "Getting data for " << id << " from uploadData" << std::endl;
						copy(*it);
						copyfound=true;
						it = conn->uploadData.erase(it);
						break;
					}
				if (!copyfound)
					thread = std::thread(&Client::getData, this);
				authresult=0;
			}
			else if (authresult==2) {
				conn->packet.clear();
				conn->packet << packetid;
				packetid=0;
				conn->packet << packetid;
				conn->send(*this);
				authresult=0;
			}
			else {
				thread = std::thread(&Client::authUser, this);
				authresult=0;
			}
		}
	}
}

void Client::sendLines() {
	if (incLines>=1) {
		sf::Uint8 amount=0;
		while (incLines>=1) {
			amount++;
			incLines-=1.0f;
		}
		conn->packet.clear(); // 10-Packet
		sf::Uint8 packetid = 10;
		conn->packet << packetid << amount;
		conn->send(*this);
	}
}