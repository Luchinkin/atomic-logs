#pragma once
#include <array>
#include <atomic>
#include <string>

static constexpr uint32_t logs_buffer_size = 1024u;
static constexpr uint32_t logs_max_count = 400u;

static_assert( logs_buffer_size % 8 == 0 );

struct log_buffer_t
{
	static constexpr uint32_t mem_parts_count = logs_buffer_size / 8;
	std::array<std::atomic_uint64_t, mem_parts_count> mem;

	void fill( const char* buf, size_t size ) noexcept
	{
		size_t idx = 0;

		while ( size > 0 )
		{
			uint64_t tmp = 0;
			memcpy( &tmp, buf, std::min<size_t>( size, 8u ) );
			mem[ idx ].store( tmp );

			if ( size >= 8 )
				size -= 8;
			else
				size = 0;

			idx++;
			buf += 8;
		}
	}

	std::string str() const noexcept
	{
		char buffer[ logs_buffer_size ];
		char* tmp_ptr = buffer;
		for ( auto& m : mem )
		{
			uint64_t tmp = m.load();
			memcpy( tmp_ptr, &tmp, 8 );
			tmp_ptr += 8;
		}
		return buffer;
	}
};

using log_buffers_t = std::array<log_buffer_t, logs_max_count>;

struct atomic_logs_t
{
	std::atomic_uint32_t log_idx = 0;
	log_buffers_t buffers;

	static constexpr size_t last_valid_log_idx = logs_max_count - 1;

	void push( const char* buf, size_t size ) noexcept
	{
		if ( log_idx == logs_max_count )
		{
			auto copy_buffer_mem = []( auto& dst, auto& src )
			{
				for ( size_t i = 0; i < log_buffer_t::mem_parts_count; i++ )
				{
					dst.mem[ i ].store( src.mem[ i ].load() );
				}
			};

			for ( size_t i = 0; i < ( logs_max_count - 1 ); i++ )
			{
				copy_buffer_mem( buffers[ i ], buffers[ i + 1 ] );
			}

			buffers[ last_valid_log_idx ].fill( buf, size );
		}
		else
		{
			buffers[ log_idx++ ].fill( buf, size );
		}
	}

	void pop( log_buffers_t& out ) noexcept
	{
		auto atomic_copy_buffer_and_reset = [this, &out]( size_t idx )
		{
			auto& out_buf = out[ idx ];
			auto& in_buf = buffers[ idx ];

			for ( size_t i = 0; i < log_buffer_t::mem_parts_count; i++ )
			{
				out_buf.mem[ i ].store( in_buf.mem[ i ].load() );
				in_buf.mem[ i ].store( 0 );
			}
		};

		for ( size_t i = 0; i < logs_max_count; i++ )
		{
			atomic_copy_buffer_and_reset( i );
		}

		log_idx = 0;
	}
};

inline atomic_logs_t g_atomic_logs{};
