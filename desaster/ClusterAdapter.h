#ifndef desaster_ClusterAdapter_h
#define desaster_ClusterAdapter_h

#include <desaster/Module.h>
#include <string>
#include <list>
#include <unordered_map>
#include <ev++.h>

/*! Implements clustering to Desaster.
 */
class ClusterAdapter :
	public Module
{
public:
	class Connection { // {{{
	public:
		Connection(ClusterAdapter* owner, int fd);
		~Connection();

	private:
		void io(ev::io& io, int revents);
	}; // }}}

private:
	std::string groupName_;
	std::string bindAddress_;
	std::string brdAddress_;
	int port_;

	// (UDP) peer broadcasting
	ev::io peeringWatcher_;
	ev::timer peeringTimer_;
	int peeringAttemptTimeout_;
	int peeringAttemptCount_;
	int peeringAttemptMax_;

	// (TCP) inter-communication (protocol shall be binary, ideally)
	ev::io syncWatcher_;
	std::list<Connection*> connections_;

	// cluster TCP command map
	std::unordered_map<std::string, void (ClusterAdapter::*)(Connection* node, const char* args)> commands_;

public:
	explicit ClusterAdapter(Server* server);
	~ClusterAdapter();

	const std::string groupName() const { return groupName_; }
	const std::string& brdAddress() const { return brdAddress_; }
	const std::string address() const { return bindAddress_; }
	int port() const { return port_; }

	virtual bool start();
	virtual void stop();

private:
	bool setupPeeringListener();
	bool searchPeers();
	void peeringTimeout(ev::timer&, int);
	void becomeMaster();
	void becomeSlave();

	void peering(ev::io& listener, int revents);

	void hi(Connection* c, const std::string& args);
	void bye(Connection* c, const std::string& args);
	void status(Connection* c, const std::string& args);
	void sync(Connection* c, const std::string& args);
};

#endif
