// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

class Player
{
public:
    enum class Winner
    {
        kNone,
        kP1,
        kP2,
    };

public:
    Player() = default;

	void SetWinner(Winner w) { m_winner = w; }
	Winner GetWinner() const { return m_winner; }

private:
	Winner m_winner = Winner::kNone;
};