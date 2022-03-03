#include "EventHandler.hpp"

EventHandler::EventHandler()
{
    if ((kq = kqueue()) == -1)
        TERMINATE("'kqueue' failed in EventHandler");
}

EventHandler::~EventHandler()
{

}

EventHandler::EventHandler(const EventHandler &other)
{
    *this = other;
}

EventHandler &EventHandler::operator=(const EventHandler &other)
{
    if (this != &other)
    {
        kq = other.kq;
        event = other.event;
        for (unsigned int i = 0; i < MAX_EVENTS; ++i)
            evList[i] = other.evList[i];
    }
    return (*this);
}

void EventHandler::addReadEvent(int socket, int *userDefinedData)
{
    // EV_SET() -> initialize a kevent structure, here our listening server
    /* No EV_CLEAR, otherwise when backlog is full, the connection request has to
    * be repeated.. instead let them hang in the queue until they are served
    */
    EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, userDefinedData);
    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1)
        TERMINATE("'kevent' failed in EventHandler");
}

void EventHandler::addTimeEvent(int socket, int time_in_ms, int *userDefinedData)
{
    EV_SET(&event, socket, EVFILT_TIMER, EV_ADD, 0, time_in_ms, userDefinedData);
    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1)
        TERMINATE("'kevent' failed in EventHandler");
}

void EventHandler::removeReadEvent(int socket)
{
    EV_SET(&event, socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1)
        TERMINATE("'kevent' failed in EventHandler");
}

void EventHandler::removeTimeEvent(int socket)
{
    EV_SET(&event, socket, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1)
        TERMINATE("'kevent' failed in EventHandler");
}
