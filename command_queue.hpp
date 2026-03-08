// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <queue>
#include "command.hpp"

class CommandQueue
{
public:
	void Push(const Command& command);
	Command Pop();
	bool IsEmpty() const;

private:
	std::queue<Command> m_queue;
};

