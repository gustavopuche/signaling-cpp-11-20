#include <chrono>
#include <iostream>
#include <mutex>
#include <ostream>
#include <queue>
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

ostream &operator<<(ostream &os, MessageType m) {
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

mutex m_inbox;
queue<Message> inbox;

mutex m_outbox;
Message outbox;

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
  Signaler(int n) : stop_after_logout_count(n) {}
  void operator()(){
    cout << "Starting Signaler with logout count "
         << stop_after_logout_count << endl;
    while(stop_after_logout_count > 0)
    {
      GetSignal();
      this_thread::sleep_for(seconds{1});
    }

  }
private:
  int stop_after_logout_count;

  void GetSignal();
  void AnswareLogin(Message& m);
  void AnswareCredentials(Message& m);
  void AnswareLogout(Message& m);
};

void Signaler::GetSignal() {
  cout << "Signaler::GetSignal..." << endl;
  if (!inbox.empty())
  {
    scoped_lock lck{m_inbox};
    Message m = inbox.front();
    cout << "Signal read id: " << m.id << " type: " << m.type << endl;

    // WARNING: inside a lock we are getting another lock.
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
    inbox.pop();
  }
}

void Signaler::AnswareLogin(Message &m) {
  while (outbox.id != -1)
  {
    cout << "Signaler is waiting outbox is read in AnswareLogin" << endl;
    this_thread::sleep_for(milliseconds{20});
  }
  scoped_lock lck{m_outbox};
  outbox.id = m.id;
  outbox.type = MessageType::login_rec;
}

void Signaler::AnswareCredentials(Message &m) {
  while (outbox.id != -1)
  {
    cout << "Signaler is waiting outbox is read in AnswareCredentials" << endl;
    this_thread::sleep_for(milliseconds{20});
  }
  scoped_lock lck{m_outbox};
  outbox.id = m.id;
  outbox.type = MessageType::credentials_rec;
}

void Signaler::AnswareLogout(Message &m) {
  while (outbox.id != -1)
  {
    cout << "Signaler is waiting outbox is read in AnswareLogout" << endl;
    this_thread::sleep_for(milliseconds{20});
  }
  scoped_lock lck{m_outbox};
  outbox.id = m.id;
  outbox.type = MessageType::logout_rec;
  stop_after_logout_count--; // Decrement logout cont to stop execution.
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
  User(int id, string name) : id(id), name(name){}

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
  scoped_lock lck{m_inbox}; // acquire inbox mutex.
  cout << "User id: " << id << " name: " << name << " sends login." << endl;
  inbox.push(m);
}

void User::SendCredentials() {
  Message m;
  m.id = id;
  m.type = MessageType::credentials;
  m.params.push_back(Param{"user",name});
  m.params.push_back(Param{"password",name+"_passwd"});
  scoped_lock lck{m_inbox}; // acquire inbox mutex.
  cout << "User id: " << id << " name: " << name
       << " sends credentials." << endl;
  inbox.push(m);
}

void User::SendLogout() {
  Message m;
  m.id = id;
  m.type = MessageType::logout;
  scoped_lock lck{m_inbox}; // acquire inbox mutex.
  cout << "User id: " << id << " name: " << name << " sends logout." << endl;
  inbox.push(m);
}

bool User::GetAnsware() {
  scoped_lock lck{m_inbox};
  if(outbox.id == id)
  {
    outbox.id = -1;
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
  outbox.id = -1; // Initialize outbox to empty.

  int n = 10;
  thread login_signaler{Signaler{n}};

  vector<thread> vth;
  for (int i = 1; i <= 10; i++)
  {
    vth.push_back(thread{User{i,"User"+to_string(i)}});
  }

  login_signaler.join();

  for (int i = 0; i < 10; i++)
  {
    vth[i].join();
  }

  return 0;
}
