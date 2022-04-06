#ifndef EVENTHANDLER_HPP
# define EVENTHANDLER_HPP

# include "header.hpp"

# define MAX_EVENTS 128 // for kqueue 

class EventHandler
{
public:
    EventHandler();
    ~EventHandler();
    EventHandler(const EventHandler &other);
    EventHandler &operator=(const EventHandler &other);

    void addReadEvent(int socket, int *userDefinedData = NULL);
    void addTimeEvent(int socket, int time_in_ms, int *userDefinedData = NULL);
    void removeReadEvent(int socket);
    void removeTimeEvent(int socket);

    /* returns number of events */
    inline int getNumberOfEvents(void) const
    {
        int nOfEvents;
        if ((nOfEvents = kevent(kq, NULL, 0, (struct kevent *)evList, MAX_EVENTS, NULL)) == -1)
            TERMINATE("'kevent' failed in EventHandler");
        return (nOfEvents);
    }

    struct kevent &operator[](unsigned int index)
    {
        if (index >= MAX_EVENTS)
            throw (std::out_of_range("Requested event is outside of the range of the event queue"));
        return (evList[index]);
    }
private:
    int             kq; /* holds all the events we are interested in */
    struct kevent   event;
    struct kevent   evList[MAX_EVENTS];
};

#endif
