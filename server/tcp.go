package server

import (
	"bufio"
	"encoding/binary"
	"errors"
	"glory/message"
	"io"
	"log"
	"net"
	"runtime"
	"time"
)

// ErrServerClosed is returned by the Server's Serve, ListenAndServe after a call to Shutdown or Close.
var ErrServerClosed = errors.New("server closed")

const (
	// ReaderBuffsize is used for bufio reader.
	ReaderBuffsize = 1024
)

func (s *Server) getDoneChan() <-chan struct{} {
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.doneChan == nil {
		s.doneChan = make(chan struct{})
	}
	return s.doneChan
}

func (s *Server) tcpListener(ln net.Listener) error {
	var tempDelay time.Duration

	s.mu.Lock()
	s.ln = ln
	if s.activeConn == nil {
		s.activeConn = make(map[net.Conn]struct{})
	}
	s.mu.Unlock()

	for {
		conn, e := ln.Accept()
		if e != nil {
			select {
			case <-s.getDoneChan():
				return ErrServerClosed
			default:
			}

			if ne, ok := e.(net.Error); ok && ne.Temporary() {
				if tempDelay == 0 {
					tempDelay = 5 * time.Millisecond
				} else {
					tempDelay *= 2
				}

				if max := 1 * time.Second; tempDelay > max {
					tempDelay = max
				}

				log.Printf("Accept error: %v; retrying in %v", e, tempDelay)
				time.Sleep(tempDelay)
				continue
			}
			return e
		}
		tempDelay = 0

		if tc, ok := conn.(*net.TCPConn); ok {
			tc.SetKeepAlive(true)
			tc.SetKeepAlivePeriod(3 * time.Minute)
		}

		s.mu.Lock()
		s.activeConn[conn] = struct{}{}
		s.mu.Unlock()

		go s.tcpConn(conn)
	}
}

func (s *Server) tcpConn(conn net.Conn) {

	defer func() {
		if err := recover(); err != nil {
			const size = 64 << 10
			buf := make([]byte, size)
			ss := runtime.Stack(buf, false)
			if ss > size {
				ss = size
			}
			buf = buf[:ss]
			log.Printf("serving %s panic error: %s, stack:\n %s\n", conn.RemoteAddr(), err, buf)
		}
		s.mu.Lock()
		delete(s.activeConn, conn)
		s.mu.Unlock()
		conn.Close()
	}()

	r := bufio.NewReaderSize(conn, ReaderBuffsize)

	for {
		t0 := time.Now()
		if s.readTimeout != 0 {
			conn.SetReadDeadline(t0.Add(s.readTimeout))
		}

		name, params, err := message.Decode(r)
		if err != nil {
			if err == io.EOF {
				log.Printf("client has closed this connection: %s\n", conn.RemoteAddr().String())
				return
			}
			log.Println(err)
			return
		}

		if s.writeTimeout != 0 {
			conn.SetWriteDeadline(t0.Add(s.writeTimeout))
		}

		go func() {
			response, err := s.Call(string(name), params)
			var rb []byte
			if err != nil {
				rb = []byte(err.Error())
			} else {
				rb = response.Bytes()
			}

			//encode
			ssL := len(rb)
			totalL := ssL + 4

			data := make([]byte, totalL)
			copy(data[4:], rb)

			binary.BigEndian.PutUint32(data[:4], uint32(ssL))
			conn.Write(data)
		}()
	}
}
