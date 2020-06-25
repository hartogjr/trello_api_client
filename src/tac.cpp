/* BSD 3-Clause License
 *
 * Copyright (c) 2020, Simon de Hartog
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * vim:set ts=4 sw=4 noet: */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <jsoncpp/json/json.h>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>

// This is the API key used by this application
#define APIKEY "4252be3f9940f267ff366332859bcb2f"

using std::cout, std::endl;

int help()
{
	cout << "Usage: tac <Trello Token>" << endl << endl;
	cout << "For now, this application removes all archived Trello cards" << endl;
	return 1;
}

void checkrsp(const RestClient::Response & rsp_i)
{
	if (rsp_i.code >= 300) {
		throw std::runtime_error("Request failed");
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) return help();

	RestClient::init();
	atexit(RestClient::disable);

	// Ensures proper deletion upon exit
	std::unique_ptr<RestClient::Connection> rcc;
	RestClient::Response rsp;

	rcc.reset(new RestClient::Connection("https://api.trello.com/1/"));

	std::string uri;

	uri = "members/me/boards?key=" APIKEY "&token=";
	uri += argv[1];

	rsp = rcc->get(uri);
	checkrsp(rsp);

	std::stringstream sss;
	sss.str(rsp.body);
	Json::Value data;
	sss >> data;

	for (uint16_t i = 0; i < data.size(); i++) {
		cout << "Found " << (data[i]["closed"].asBool()?"closed":"open") << " board ";
		cout << data[i]["name"] << " with ID " << data[i]["id"] << endl;

		uri = "boards/";
		uri += data[i]["id"].asString();
		uri += "?cards=closed&card_fields=id,name,closed&key=" APIKEY "&token=";
		uri += argv[1];
		rsp = rcc->get(uri);
		checkrsp(rsp);

		sss.str(rsp.body);
		Json::Value brdinfo;
		sss >> brdinfo;
		Json::Value cards = brdinfo["cards"];
		for (uint16_t j = 0; j < cards.size(); j++) {
			cout << "Found " << (cards[j]["closed"].asBool()?"closed":"open");
			cout << " card \"" << cards[j]["name"].asString() << "\"";
			cout << " with ID " << cards[j]["id"];

			// Extra check, although we already filtered on closed cards
			if (cards[j]["closed"].asBool()) {
				cout << ", deleting it" << endl;
				uri = "cards/";
				uri += cards[j]["id"].asString();
				uri += "?key=" APIKEY "&token=";
				uri += argv[1];
				rsp = rcc->del(uri);
				checkrsp(rsp);
			} else {
				cout << ", still open so leaving it untouched" << endl;
			}
		}

	}

	return 0;
}
