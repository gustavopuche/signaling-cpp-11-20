#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

using namespace std;

using Param = pair<string, string>;

enum class MessageType {
  login,
  credentials,
  logout,
  login_rec,
  credentials_rec,
  logout_rec};

ostream &operator<<(ostream &os, MessageType& m) {
  switch(m)
  {
  case MessageType::login:
    os<<"login";
    break;
  case MessageType::credentials:
    os<<"credentials";
    break;
  case MessageType::logout:
    os<<"logout";
    break;
  case MessageType::login_rec:
    os<<"login_rec";
    break;
  case MessageType::credentials_rec:
    os<<"credentials_rec";
    break;
  case MessageType::logout_rec:
    os<<"logout_rec";
    break;
  default:
    os<<"Invalid MessageType";
  }
  return os;
}

struct Message {
  int           id;             // Id of the sender.
  MessageType   type;           // Type of the message.
  vector<Param> params;         // Vector of parameters.
};


enum class BoxType {inbox,outbox}; // Type of message destination.

////////////////////////////////////////////////////////////////////////////////
/// Basic communicate class.
///
/// Manage access to inbox and outbox using mutexes.
/// Implements interface send/receive.
////////////////////////////////////////////////////////////////////////////////
class Communicator {
public:
  Communicator() {
    outbox.id = -1; // Initialize outbox to empty.
  }
  void    Send(Message& m);     // Sends message to inbox or outbox.
  Message Pop();                // Pops inbox.
  Message Receive(int dest_id); // Gets message to a destiantion id.
private:
  mutex              m_inbox;   // Mutex for inbox.
  shared_mutex       m_outbox;  // Read operations not blocked unless write.
  condition_variable m_cond;    // the variable communicating events.

  queue<Message> inbox;         // Incomming messages into queue.
  Message        outbox;        // Outcomming message variable.

  void SendInbox(Message& m);   // Sends message to inbox.
  bool SendOutbox(Message& m);  // Sends message to outbox.
};

Message Communicator::Pop() {
  unique_lock lck{m_inbox};
  m_cond.wait(lck,
              [this](){return !inbox.empty();}); // release lck and wait
                                                 // re-acquire lck upon wake up
                                                 // don't wak up unless inbox
                                                 // is non-empty

  auto m = inbox.front();
  inbox.pop();

  return m;
}

void Communicator::Send(Message& m) {
  switch(m.type)
  {
  case MessageType::login:
  case MessageType::credentials:
  case MessageType::logout:
    SendInbox(m);
    break;
  case MessageType::login_rec:
  case MessageType::credentials_rec:
  case MessageType::logout_rec:
    while(!SendOutbox(m))
      this_thread::sleep_for(milliseconds{20});
    break;
  default:
    cerr << "Message type unrecognized... << " << endl;
  }
}

void Communicator::SendInbox(Message& m) {
  unique_lock lck{m_inbox};
  inbox.push(m);
}

bool Communicator::SendOutbox(Message& m) {
  unique_lock lck{m_outbox};
  if (outbox.id != -1) // Fixed error: Other thread have changed outbox.
    return false;

  outbox.id     = m.id;
  outbox.type   = m.type;
  outbox.params = m.params;

  return true;
}

Message Communicator::Receive(int dest_id) {
  shared_lock s_lck{m_outbox};
  if (outbox.id != dest_id)
  {
    Message m;
    m.id = -1; // The reply Message has not been received yet.
    return m;
  }

  s_lck.unlock(); // Needed to avoid a deadlock getting 2 locks to same mutex.

  unique_lock u_lck{m_outbox};
  Message m = outbox;
  outbox.id = -1; // Setup outbox slot as clear, ready to store new message.
  return m;
}
////////////////////////////////////////////////////////////////////////////////
/// Basic login thread class.
///
/// Handles login and logout service requests.
/// A message queue called inbox is managed by m_inbox mutex.
/// Message response is stored in outbox Message variable managed by
/// m_outbox mutex.
/// outbox is clear when id is equal to -1.
////////////////////////////////////////////////////////////////////////////////
class Signaler {
public:
  Signaler(int n,shared_ptr<Communicator> com)
    : stop_after_logout_count(n),
      com(com) {}
  void operator()(){
    cout << "Starting Signaler with logout count "
         << stop_after_logout_count << endl;

    while(stop_after_logout_count > 0)
    {
      Answare(Get());
      // this_thread::sleep_for(milliseconds{20});
    }

  }
private:
  shared_ptr<Communicator> com;
  int stop_after_logout_count;
  Message mess;

  Message& Get();
  void Answare(Message& m);
  void AnswareLogin(Message& m);
  void AnswareCredentials(Message& m);
  void AnswareLogout(Message& m);
};

Message& Signaler::Get() {
  mess = com->Pop();
  cout << "Signaler::GetSignal from id: " << mess.id << endl;
  return mess;
}

void Signaler::Answare(Message &m) {
  switch(m.type)
  {
  case MessageType::login:
    AnswareLogin(m);
    break;
  case MessageType::credentials:
    AnswareCredentials(m);
    break;
  case MessageType::logout:
    AnswareLogout(m);
    break;
  }
}

void Signaler::AnswareLogin(Message &m) {
  m.type = MessageType::login_rec;
  com->Send(m);
}

void Signaler::AnswareCredentials(Message &m) {
  m.type = MessageType::credentials_rec;
  com->Send(m);
}

void Signaler::AnswareLogout(Message &m) {
  m.type = MessageType::logout_rec;
  com->Send(m);
  stop_after_logout_count--;
  cout << "Logout count " << stop_after_logout_count << endl;
}

////////////////////////////////////////////////////////////////////////////////
/// Custom user thread class.
///
/// Asks Signaler for login, credentials and logout.
/// When User reads Signaler response puts outbox.id to -1.
////////////////////////////////////////////////////////////////////////////////
class User {
public:
  User(int id, string name,shared_ptr<Communicator> com)
    : id(id),
      name(name),
      com(com) {}

  void operator()(){
    cout << "Starting User id: " << id << " name: " << name << endl;
    SendLogin();
    Get();
    SendCredentials();
    Get();
    SendLogout();
    Get();
  }
private:
  int    id;                    // Id of the user.
  string name;                  // Name of the user.
  shared_ptr<Communicator> com; // Communicator object.

  void SendLogin();
  void SendCredentials();
  void SendLogout();

  void Get();
  bool GetAnsware();
};

void User::SendLogin() {
  Message m;
  m.id = id;
  m.type = MessageType::login;
  cout << "User id: " << id << " name: " << name << " sends login." << endl;
  com->Send(m);
}

void User::SendCredentials() {
  Message m;
  m.id = id;
  m.type = MessageType::credentials;
  m.params.push_back(Param{"user",name});
  m.params.push_back(Param{"password",name+"_passwd"});
  cout << "User id: " << id << " name: " << name
       << " sends credentials." << endl;
  com->Send(m);
}

void User::SendLogout() {
  Message m;
  m.id = id;
  m.type = MessageType::logout;
  cout << "User id: " << id << " name: " << name << " sends logout." << endl;
  com->Send(m);
}

bool User::GetAnsware() {
  if(com->Receive(id).id == id)
  {
    cout << "Get answare id: " << id << " name: " << name << endl;
    return true;
  }

  return false;
}

void User::Get() {
  while(!GetAnsware())
  {
    this_thread::sleep_for(milliseconds{20});
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// MAIN FUNCTION
//
////////////////////////////////////////////////////////////////////////////////
int main()
{
  shared_ptr c = make_shared<Communicator>();

  int n = 10;
  thread login_signaler{Signaler{n,c}};

  vector<thread> vth;
  for (int i = 1; i <= n; i++)
  {
    vth.push_back(thread{User{i,"User"+to_string(i),c}});
  }

  login_signaler.join();

  for (int i = 0; i < n; i++)
  {
    vth[i].join();
  }

  return 0;
}
