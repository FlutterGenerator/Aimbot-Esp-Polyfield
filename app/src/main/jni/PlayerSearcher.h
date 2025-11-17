vector<void*> players;

void clearPlayers() {
    vector<void*> pls;
    for (int i = 0; i < players.size(); i++) {
        if (players[i] != NULL) {
            pls.push_back(players[i]);
        }
    }
    players = pls;
}

bool playerFind(void *pl) {
    if (pl != NULL) {
        for (int i = 0; i < players.size(); i++) {
            if (pl == players[i]) return true;
        }
    }
    return false;
}