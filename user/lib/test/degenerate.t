/*
 * Here are templates for events that have no variable data.
 */
facility "user"; event_type 4101;
format
Quarterback misread coverage.
END
facility "user"; event_type 4102;
format
Tailback ran before he had the ball.
END
facility "user"; event_type 4103;
format
Holder fumbled snap.
END
facility "user"; event_type 4104;
format
Cornerback was playing too far off the receiver.
END
facility "user"; event_type 4105;
format
Wide receiver ran wrong route.
END

/*
 * Here are templates for events that have no variable data but const
 * attributes.
 */
facility "user"; event_type 4111;
const { string category = "server"; }
format
Server starting...
END
facility "user"; event_type 4112;
const { string category = "server"; }
format
Server shutting down...
END
facility "user"; event_type 4113;
const { string category = "client"; }
format
Client connecting...
END
facility "user"; event_type 4114;
const { string category = "client"; }
format
Client disconnecting...
END
facility "user"; event_type 4115;
const { string category = "client"; }
format
My client pleads not guilty, your honor.
END
facility "user"; event_type 4116;
const { string category = "server"; }
format
Server would get a bigger tip if he would actually write down our order
rather than trying to impress us by memorizing it.
END

/*
 * Here are templates for events that have STRING format and const
 * attributes.
 */
facility "user"; event_type 4121;
const { string category = "client"; }
attributes { string msg; }
format
category=%category%
msg=%msg%
END
facility "user"; event_type 4122;
const { string category = "client"; }
attributes { string msg; }
format
category=%category%
msg=%msg%
END
