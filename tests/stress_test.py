import asyncio
import sys
import time
import json

class StressTest:
    def __init__(self, host, port, num_senders, num_receivers, message_rate, duration):
        self.host = host
        self.port = port
        self.num_senders = num_senders
        self.num_receivers = num_receivers
        self.message_rate = message_rate
        self.duration = duration
        self.received_counts = [0] * num_receivers

    async def sender_client(self, client_id):
        try:
            reader, writer = await asyncio.open_connection(self.host, self.port)
            print(f"Sender {client_id} connected")
            msg_count = 0
            max_messages = int(self.message_rate * self.duration)

            while msg_count < max_messages:
                msg = f"Client {client_id}: Message {msg_count}\n".encode()
                writer.write(msg)
                await writer.drain()
                msg_count += 1
                await asyncio.sleep(1 / self.message_rate)

            writer.close()
            await writer.wait_closed()
            print(f"Sender {client_id} sent {msg_count} messages and disconnected")
        except Exception as e:
            print(f"Sender {client_id} error: {e}")

    async def receiver_client(self, client_id):
        try:
            reader, writer = await asyncio.open_connection(self.host, self.port)
            print(f"Receiver {client_id} connected")
            start_time = time.time()

            while time.time() - start_time < self.duration:
                data = await asyncio.wait_for(reader.readline(), timeout=self.duration)
                if data:
                    self.received_counts[client_id] += 1
                else:
                    break

            writer.close()
            await writer.wait_closed()
            print(f"Receiver {client_id} received {self.received_counts[client_id]} messages")
        except asyncio.TimeoutError:
            print(f"Receiver {client_id} finished with {self.received_counts[client_id]} messages")
        except Exception as e:
            print(f"Receiver {client_id} error: {e}")

    async def run_test(self):
        sender_tasks = [self.sender_client(i) for i in range(self.num_senders)]
        receiver_tasks = [self.receiver_client(i) for i in range(self.num_receivers)]
        await asyncio.gather(*sender_tasks, *receiver_tasks)

        total_expected = self.num_senders * self.message_rate * self.duration
        results = {
            "config": {
                "num_senders": self.num_senders,
                "num_receivers": self.num_receivers,
                "message_rate": self.message_rate,
                "duration": self.duration
            },
            "expected_per_receiver": total_expected,
            "actual_received": self.received_counts
        }
        print(json.dumps(results))

def main():
    if len(sys.argv) != 5:
        print("Usage: python stress_test.py <num_senders> <num_receivers> <message_rate> <duration>")
        sys.exit(1)

    num_senders = int(sys.argv[1])
    num_receivers = int(sys.argv[2])
    message_rate = float(sys.argv[3])
    duration = float(sys.argv[4])

    tester = StressTest("127.0.0.1", 8080, num_senders, num_receivers, message_rate, duration)
    asyncio.run(tester.run_test())

if __name__ == "__main__":
    main()
