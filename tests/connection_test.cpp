#include <vector>
#include <sstream>

#include <gtest/gtest.h>

#include "../src/common/connection/connection.h"

TEST(connection, network)
{
	Listener lst("127.0.0.1", 27000);
	ASSERT_TRUE(lst.OpenSocket());
	Connector conn;
	Connection node1;
	Connection node2;

	ASSERT_TRUE(!node1.isValid());
	ASSERT_TRUE(!node2.isValid());

	#pragma omp parallel for
	for (int i = 0; i < 2; ++i) {
		if (i == 0) {
			sleep(2);
			node1 = conn.Connect("127.0.0.1", 27000);
		} else {
			node2 = lst.Accept();
		}
	}

	ASSERT_TRUE(node1.isValid());
	ASSERT_TRUE(node2.isValid());

	std::vector<char> data(10);

	for (size_t idx = 0; idx < data.size(); ++idx) {
		data[idx] = idx;
	}

	node1.Send(data);
	std::vector<char> testData = node2.Receive();

	std::stringstream ss;

	for (char value : data) {
		ss << int(value) << " ";
	}

	ss << "\n";

	for (char value : testData) {
		ss << int(value) << " ";
	}

	ss << "\n";

	ASSERT_TRUE(data == testData) << ss.str();;
	ASSERT_TRUE(node1.isValid());
	ASSERT_TRUE(node2.isValid());

	node2.Close();
	node1.Receive();

	ASSERT_TRUE(!node1.isValid());
	ASSERT_TRUE(!node2.isValid());

	lst.CloseSocket();
}

TEST(connection, pipes)
{
	Connection node1;
	Connection node2;

	ASSERT_TRUE(!node1.isValid());
	ASSERT_TRUE(!node2.isValid());

	std::pair<Connection, Connection> conns = Listener::GetPipe();
	node1 = conns.first;
	node2 = conns.second;

	ASSERT_TRUE(node1.isValid());
	ASSERT_TRUE(node2.isValid());

	std::vector<char> data(10);

	for (size_t idx = 0; idx < data.size(); ++idx) {
		data[idx] = idx;
	}

	node1.Send(data);
	std::vector<char> testData = node2.Receive();

	std::stringstream ss;

	for (char value : data) {
		ss << int(value) << " ";
	}

	ss << "\n";

	for (char value : testData) {
		ss << int(value) << " ";
	}

	ss << "\n";

	ASSERT_TRUE(data == testData) << ss.str();;
	ASSERT_TRUE(node1.isValid());
	ASSERT_TRUE(node2.isValid());

	node2.Close();
	node1.Receive();

	ASSERT_TRUE(!node1.isValid());
	ASSERT_TRUE(!node2.isValid());
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
