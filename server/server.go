package server

import (
	"errors"
	"log"
	"net"
	"reflect"
	"sync"
	"time"
)

// Server is rpc server that use TCP.
type Server struct {
	ln           net.Listener
	readTimeout  time.Duration
	writeTimeout time.Duration

	serviceMapMu sync.RWMutex
	serviceMap   map[string]*service

	mu         sync.RWMutex
	activeConn map[net.Conn]struct{}
	doneChan   chan struct{}
}

type service struct {
	name string        // name of service
	rcvr reflect.Value // receiver of methods for the service
	typ  reflect.Type  // type of the receiver
}

// Start is start server.
func (s *Server) Start(network, address string) (err error) {
	var ln net.Listener
	ln, err = s.makeListener(network, address)

	if err != nil {
		log.Println(err)
		return
	}

	//tcp network
	if "tcp" == network {
		return s.tcpListener(ln)
	}

	return nil
}

// Register is register a service.
func (s *Server) Register(name string, rcvr interface{}) error {
	s.serviceMapMu.Lock()
	defer s.serviceMapMu.Unlock()
	if s.serviceMap == nil {
		s.serviceMap = make(map[string]*service)
	}

	service := new(service)
	service.name = name
	service.typ = reflect.TypeOf(rcvr)
	service.rcvr = reflect.ValueOf(rcvr)

	s.serviceMap[service.name] = service
	return nil
}

// Call is call a method.
func (s *Server) Call(name string, argv []interface{}) (rtv reflect.Value, err error) {

	//call function
	s.serviceMapMu.RLock()
	service := s.serviceMap[name]
	s.serviceMapMu.RUnlock()

	if service == nil {
		err = errors.New("did not register the service")
		return
	}

	var fn reflect.Value
	if service.typ.Kind() == reflect.Func {
		fn = service.rcvr
	} else {
		fname := argv[0].(string)
		argv = argv[1:]
		fn = service.rcvr.MethodByName(fname)
	}

	if fn.Kind() == reflect.Invalid {
		err = errors.New("the method name wrong")
		return
	}

	//parameters of the judgment
	t := fn.Type()

	if len(argv) != t.NumIn() {
		err = errors.New("parameter length is not correct")
		return
	}

	if t.NumOut() != 1 {
		err = errors.New("too many output parameters")
		return
	}

	//The return type of the method must be byte.
	if returnType := t.Out(0); returnType != reflect.TypeOf((*[]byte)(nil)).Elem() {
		err = errors.New("the return type of the method must be byte")
		return
	}

	//params
	var rfus []reflect.Value
	for _, arg := range argv {
		rfus = append(rfus, reflect.ValueOf(arg))
	}

	rtv = fn.Call(rfus)[0]
	return
}
