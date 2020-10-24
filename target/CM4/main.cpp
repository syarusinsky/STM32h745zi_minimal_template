int main(void)
{
	unsigned int someVar = 7;
	while ( true )
	{
		someVar += 3;
		
		while ( someVar % 2 == 1 )
		{
			someVar /= 2;
		}
	}
}
