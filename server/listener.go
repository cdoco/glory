package server

import (
	"fmt"
	"net"
)

var makeListeners = make(map[string]MakeListener)

func init() {
	makeListeners["tcp"] = tcpMakeListener
}

// RegisterMakeListener registers a network.
func RegisterMakeListener(network string, ml MakeListener) {
	makeListeners[network] = ml
}

// MakeListener defines a listener generater.
type MakeListener func(s *Server, address string) (ln net.Listener, err error)

func (s *Server) makeListener(network, address string) (ln net.Listener, err error) {
	ml := makeListeners[network]
	if ml == nil {
		return nil, fmt.Errorf("can not make listener for %s", network)
	}
	return ml(s, address)
}

func tcpMakeListener(s *Server, address string) (ln net.Listener, err error) {
	return net.Listen("tcp", address)
}
