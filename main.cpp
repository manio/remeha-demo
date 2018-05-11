#include<iostream>              //cout
#include<stdio.h>               //printf
#include<string.h>              //strlen
#include<string>                //string
#include<sys/socket.h>          //socket
#include<arpa/inet.h>           //inet_addr
#include<netdb.h>               //hostent
#include<thread>
#include <unistd.h>
#include <atomic>

using namespace std;

unsigned char req[] = { 0x02, 0xfe, 0x01, 0x05, 0x08, 0x02, 0x01, 0x69, 0xab, 0x03 };

char buff[80];

int sock = -1;
std::string address = "";
int port = 0;
struct sockaddr_in server;

bool conn(string address, int port)
{
  //create socket if it is not already created
  if (sock == -1)
  {
    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
      perror("Could not create socket");
    }

    cout << "Socket created\n";
  }
  else
  {                             /* OK , nothing */
  }

  //setup address structure
  if (inet_addr(address.c_str()) == -1)
  {
    struct hostent *he;
    struct in_addr **addr_list;

    //resolve the hostname, its not an ip address
    if ((he = gethostbyname(address.c_str())) == NULL)
    {
      //gethostbyname failed
      herror("gethostbyname");
      cout << "Failed to resolve hostname\n";

      return false;
    }

    //Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
    addr_list = (struct in_addr **) he->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++)
    {
      //strcpy(ip , inet_ntoa(*addr_list[i]) );
      server.sin_addr = *addr_list[i];

      cout << address << " resolved to " << inet_ntoa(*addr_list[i]) << endl;

      break;
    }
  }

  //plain ip address
  else
  {
    server.sin_addr.s_addr = inet_addr(address.c_str());
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);

  //Connect to remote server
  if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
  {
    perror("connect failed. Error");
    return false;
  }

  cout << "Connected\n";
  return true;
}

bool send_data2(unsigned char *data, int len)
{
  //Send some data
  if (send(sock, data, len, 0) < 0)
  {
    perror("Send failed : ");
    return false;
  }
  cout << "Data send\n";

  return true;
}

string receive(int size = 512)
{
  char buffer[size];
  string reply;

  //Receive a reply from the server
  if (recv(sock, buffer, sizeof(buffer), 0) < 0)
  {
    puts("recv failed");
  }

  reply = buffer;
  return reply;
}

unsigned int CRC16(unsigned char *buf, int len)
{
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
    crc ^= (unsigned int) buf[pos];     // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--)
    {                           // Loop over each bit
      if ((crc & 0x0001) != 0)
      {                         // If the LSB is set
        crc >>= 1;              // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                      // Else LSB is not set
        crc >>= 1;              // Just shift right
    }
  }

  return crc;
}

int main()
{
  char cmd[1024];
  conn("remeha", 999);
  send_data2(req, sizeof(req));
  sleep(2.5);
  int x = -1;
  int size = 0;
  do
  {
    if ((x = recv(sock, buff + size, sizeof(buff) - size, MSG_DONTWAIT)) < 0)
    {
      puts("recv failed");
      return -1;
      break;
    }
    else
    {
      printf(".", x);
      size += x;
    }
  }
  while (size < 74);
  printf("\nGot size=%d\n", size);
  for (int i = 0; i < size; i++)
    printf("%02X ", (unsigned char) buff[i]);
  printf("\n");
  unsigned int crc = CRC16((unsigned char *) &buff[1], 70);
  printf("CRC = %02X %02X\n", (unsigned char) crc & 0xff, (unsigned char) (crc >> 8));
  if ((unsigned char) buff[71] == (unsigned char) crc & 0xff && (unsigned char) buff[72] == (unsigned char) (crc >> 8))
  {
    printf("CRC OK!\n");
    char *start = &buff[7];
    unsigned int float_temp;
    unsigned int return_temp;
    unsigned int room_temp;
    unsigned int room_setpoint;
    unsigned int ch_setpoint;
    unsigned int dhw_setpoint;
    unsigned int boiler_control_temp;
    unsigned int control_temp;
    unsigned int internal_setpoint;
    unsigned int dhw_setpoint_hmi;
    unsigned int ch_setpoint_hmi;
    unsigned int pump_power;
    float_temp = ((unsigned char) start[1] << 8) + (unsigned char) start[0];
    return_temp = ((unsigned char) start[3] << 8) + (unsigned char) start[2];
    boiler_control_temp = ((unsigned char) start[13] << 8) + (unsigned char) start[12];
    room_temp = ((unsigned char) start[15] << 8) + (unsigned char) start[14];
    ch_setpoint = ((unsigned char) start[17] << 8) + (unsigned char) start[16];
    dhw_setpoint = ((unsigned char) start[19] << 8) + (unsigned char) start[18];
    room_setpoint = ((unsigned char) start[21] << 8) + (unsigned char) start[20];
    internal_setpoint = ((unsigned char) start[28] << 8) + (unsigned char) start[27];
    pump_power = (unsigned char) start[30];
    control_temp = ((unsigned char) start[52] << 8) + (unsigned char) start[51];
    ch_setpoint_hmi = (unsigned char) start[60];
    dhw_setpoint_hmi = (unsigned char) start[61];
    printf("Actual power: %d%% (0x%02X)\n", (unsigned char) start[33], (unsigned char) start[33]);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'actual_power value=%d'", (unsigned char) start[33]);
    system(cmd);
    printf("Float temp: %.2fC (0x%04X)\n", float_temp / 100.0, float_temp);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'float_temp value=%.2f'", float_temp / 100.0);
    system(cmd);
    printf("Return temp: %.2fC (0x%04X)\n", return_temp / 100.0, return_temp);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'return_temp value=%.2f'", return_temp / 100.0);
    system(cmd);
    printf("Room temp: %.2fC (0x%04X)\n", room_temp / 100.0, room_temp);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'room_temp value=%.2f'", room_temp / 100.0);
    system(cmd);
    printf("Room temp setpoint: %.2fC (0x%04X)\n", room_setpoint / 100.0, room_setpoint);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'room_temp_setpoint value=%.2f'", room_setpoint / 100.0);
    system(cmd);
    printf("CH setpoint: %.2fC (0x%04X)\n", ch_setpoint / 100.0, ch_setpoint);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'ch_setpoint value=%.2f'", ch_setpoint / 100.0);
    system(cmd);
    printf("DHW setpoint: %.2fC (0x%04X)\n", dhw_setpoint / 100.0, dhw_setpoint);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'dhw_setpoint value=%.2f'", dhw_setpoint / 100.0);
    system(cmd);
    printf("Boiler Control temp: %.2fC (0x%04X)\n", boiler_control_temp / 100.0, boiler_control_temp);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'boiler_control_temp value=%.2f'", boiler_control_temp / 100.0);
    system(cmd);
    printf("Control temp: %.2fC (0x%04X)\n", control_temp / 100.0, control_temp);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'control_temp value=%.2f'", control_temp / 100.0);
    system(cmd);
    printf("Internal setpoint: %.2fC (0x%04X)\n", internal_setpoint / 100.0, internal_setpoint);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'internal_setpoint value=%.2f'", internal_setpoint / 100.0);
    system(cmd);
    printf("CH setpoint HMI: %dC (0x%02X)\n", ch_setpoint_hmi, ch_setpoint_hmi);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'ch_setpoint_hmi value=%d'", ch_setpoint_hmi);
    system(cmd);
    printf("DHW setpoint HMI: %dC (0x%02X)\n", dhw_setpoint_hmi, dhw_setpoint_hmi);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'dhw_setpoint_hmi value=%d'", dhw_setpoint_hmi);
    system(cmd);
    printf("Pump: %d%% (0x%02X)\n", pump_power, pump_power);
    sprintf(cmd, "curl -i -XPOST --silent -o /dev/null 'http://127.0.0.1:8086/write?db=remeha' --data-binary 'pump_power value=%d'", pump_power);
    system(cmd);
  }
  else
    printf("CRC ERROR!\n");
}
