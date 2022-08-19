# Atomic logs

Logs built on top of atomic buffers without spinlocks by using 8 byte atomic operations.
The best use case is multiprocessing architecture.

## Example:
```cpp
g_atomic_logs.push( "123", 4 );
for ( int i = 0; i < 1024; i++ )
{
	auto str = std::to_string( i );
	g_atomic_logs.push( str.c_str(), str.size() );
}

log_buffers_t output;
g_atomic_logs.pop( output );
for ( const auto& buf : output )
{
	const auto str = buf.str();
	if ( !str.empty() )
		std::cout << str.c_str() << std::endl;
}
```
