<?php

function rbge_cal_denied(){
    
    $out = "<p>Sorry but you don't have access to this resource.</p>";
    
    if(request_path() == 'node/add/event' && isset($_GET['venue_tid']) && isset($_GET['date'])){
        
        // load the venue
        $venue = entity_load_single('taxonomy_term', $_GET['venue_tid']);
        
        // load the date - something of a repeat of what is in the event form
        // seconds need to be rounded down the nearest 15 minutes to fit with the UI      
        $d = new DateTime(substr($_GET['date'], 0,19));
        $mins = floor($_GET['plus_seconds'] / 60);
        $quarters = floor($mins / 15);
        $seconds = $quarters * 15 * 60;
        $d->add( new DateInterval('PT'. $seconds .'S'));
        
        // generate a mailto link
        $subject = rawurlencode('Universal Calendar Booking Enquiry');
        
        $body = "I would like to book $venue->name on " . $d->format('l jS F Y \a\t H:i');
        $body .= "\n\n";
    
        $body .= "Time and place link useful for administrator: \n\n";
        $body .= "https://" . $_SERVER['HTTP_HOST'] . "/node/add/event?" . $_SERVER['QUERY_STRING'];
        $body .= "\n\n";
        $body = rawurlencode($body);
        
        $mailto = "mailto:roombookingoffice@rbge.org.uk?subject=$subject&body=$body";
        
        // write out a nice message
        $out .= "<h2>Trying to book a venue?</h2>";
        $out .= "<p>It looks like you are trying to book";
        $out .= ' <strong><a href="taxonomy/term' . $venue->tid . '">' . $venue->name . '</a></strong>';
        $out .= ' on ' . $d->format('l jS F Y \a\t H:i');
        $out .=  ".</p><p>You need to be logged in to do this.</p>";         
        $out .= "<p>If you haven't been given access to the system you can book through your departmental PA or main reception.</p>";
        $out .= "<p><a href=". $mailto . ">Click here to create an enquiry email with the above details in it.</a></p>";
        
    }
    
    return $out;
    
}

?>