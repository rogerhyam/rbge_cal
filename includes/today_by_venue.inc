<?php

function today_by_venue_embed($year, $month, $day){
    
    echo '<style>';
    echo <<<'EOD'
    .rbge-cal-today-by-venue {
        font-family: Verdana,​sans-serif;
        font-size: 14px;
    }    
    .rbge-cal-today-by-venue .rbge-cal-venue{
        font-weight: bold;
    }
    .rbge-cal-today-by-venue .rbge-cal-event{
        font-weight: normal;
    }
    a:link, a:visited {
        color: #2f416f;
        text-decoration: none;
    }
    .rbge-cal-today-by-venue ul{
        padding-left:1em;
    }
    .rbge-cal-today-by-venue li{
        list-style-image: url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' version='1.1' height='12' width='12'><circle cx='6' cy='6' r='4' stroke='gray' stroke-width='1' fill='none' /></svg>");
    }
    .rbge-cal-today-by-venue strong{
        color: gray;
    }
    '
EOD;

    echo '</style>';
    echo today_by_venue($year, $month, $day, true);

    drupal_exit();

}

function today_by_venue($year, $month, $day, $simple = false){
    
    // we use the datepicker jquery UI library
    drupal_add_library('system', 'ui.datepicker');

    // if there is no date passed use today
    if(!$year || !$month || !$day){
        $date = new DateTime();        
    }else{
        $date = DateTime::createFromFormat("Y-m-d", "$year-$month-$day");
    }
    
    drupal_set_title($date->format('l jS F Y'));
    
    // set up the day
    $day = array();
    $day['start'] = $date->format("Y-m-d") . 'T00:00:00';
    $day['end'] = $date->format("Y-m-d") . 'T23:59:59';
    
    // and any daylight offsetting
    $daylight_saving = date('I');
    
    // we use the 
    module_load_include('inc', 'rbge_cal', 'includes/venue_event_tree');
    
    $venues = get_venue_event_tree($day);

    $roots = $venues[-1];
    unset($venues[-1]);
    
    $out = '<div class="rbge-cal-today-by-venue">';
    
    // add a jumper if we are not displaying simple
    if(!$simple) $out .= '<input type="text" size="5" value="Go to" id="today-jump-date"/>';

    $out .= '<ul>';
    foreach($roots as $root){
        render_venue($out, $root);
    }
    $out .= '</ul>';
    
    // don't do a right column if we are doing a simple render
    if($simple){
        $out .= '</div>';
        return $out;
    }
    
    if(!$simple){
        $out .= "<p class=\"rbge-cal-index-block\">
            Only venues with events today are shown.<br/>
            <span class=\"rbge-highlight-as-now\">Current events are highlighted like this.</span><br/>
            <span class=\"rbge-cal-status-13\">Cancelled events are highlighted like this.</span><br/>
            <span class=\"rbge-cal-category-203\">Alerts look like this.</span><br/>
            Times are room booking times <strong>not</strong> publicly advertised time.</p>";        
    }

    $out .= '</div>';    
    
    // a right column floated
    $out .= '<div id="rbge-cal-today-right-col">';
    
    // embed a stories panel floated right
    $out .= '<div id="rbge-cal-stories-embed">';
    $out .= "<h2><a href='http://stories.rbge.org.uk'>Botanics Stories</a></h2>";
    $out .= '<div id="rbge-cal-stories-embed-inner" >';
    $out .= '<script type="text/javascript" src="/rbge/stories/js" ></script>';
    $out .= '</div>';
    $out .= '</div>';
    
    // embed a twitter widget
    // Tweets about @thebotanics OR @InverleithHouse OR @EdibleGarden1 OR #rbgehort OR @RBGE_Science OR @LoganBotanicGdn OR @BotanicCottage OR @BotanicsStories OR @benmorebotgdn OR @dawyck
    $twitter_url = "https://twitter.com/search?q=%40thebotanics%20OR%20%40InverleithHouse%20OR%20%40EdibleGarden1%20OR%20%23rbgehort%20OR%20%40RBGE_Science%20OR%20%40LoganBotanicGdn%20OR%20%40BotanicCottage%20OR%20%40BotanicsStories%20OR%20%40benmorebotgdn%20OR%20%40dawyck";
    $out .= '<div id="rbge-cal-twitter-embed">';
    $out .= "<h2><a href='$twitter_url'>Tweets about the Botanics</a></h2>";
    $out .= '<a
        class="twitter-timeline" 
        data-chrome="noheader" href="'.$twitter_url.'" data-widget-id="693107419858243584">Tweets about @thebotanics OR @InverleithHouse OR @EdibleGarden1 OR #rbgehort OR @RBGE_Science OR @LoganBotanicGdn OR @BotanicCottage OR @BotanicsStories OR @benmorebotgdn OR @dawyck</a>
    <script>!function(d,s,id){var js,fjs=d.getElementsByTagName(s)[0],p=/^http:/.test(d.location)?\'http\':\'https\';if(!d.getElementById(id)){js=d.createElement(s);js.id=id;js.src=p+"://platform.twitter.com/widgets.js";fjs.parentNode.insertBefore(js,fjs);}}(document,"script","twitter-wjs");</script>
    ';
    $out .= '</div>';
    
    $out .= '</div>'; // end rbge-cal-today-right-col
    
    return $out;
    
}

function render_venue(&$out, $venue){
    
    // if we have no children or events they don't render
    if(!isset($venue->event_count) || $venue->event_count == 0) return;
    
    // for simple rendering we need absolute paths
    $base_uri = "http".(!empty($_SERVER['HTTPS'])?"s":""). "://".$_SERVER['SERVER_NAME'];
    
    $out .= '<li class="rbge-cal-venue">';
    $out .= '<a href="' . $base_uri . '/taxonomy/term/'.  $venue->tid .'">';
    $out .= $venue->name;
    $out .= '</a>';

    if(isset($venue->events)){
        $out .= '<ul>';
        foreach($venue->events as $event_repeat){
  
            $event = $event_repeat[0];
            $repeat = $event_repeat[1];          
            $start = new DateTime($repeat->start);
            $end = new DateTime($repeat->end);
            $status_tid = $event->field_event_status['und'][0]['tid'];
            $category_tid = $event->field_event_category['und'][0]['tid'];
            
  
            $out .= '<li class="rbge-cal-event rbge-highlight-if-now rbge-cal-status-'. $status_tid .' rbge-cal-category-'. $category_tid . '"';
            
            // machine reading time span
            $out .= ' data-start-milli="' . ($start->getTimestamp() * 1000) . '"';
            $out .= ' data-end-milli="' .   ($end->getTimestamp() * 1000)   . '"';
            
            $out .= ' >';
            
            // human reading time span
            $out .= $start->format('H:i');
            $out .= ' - ';
            $out .= $end->format('H:i');
            
            // linked title
            $out .= ' ~ <a href="'. $base_uri .'/node/' . $event->nid . '">';
            $out .= $event->title;
            $out .= '</a>';
            
            // put in the advertised times - may be multiple value
            if (
                isset( $event->field_advertised_start_time['und'] )
                &&
                count($event->field_advertised_start_time['und'] ) > 0
               ){
                
                $out .= '<br/><strong>Advertised: </strong>';
                for($i = 0; $i < count($event->field_advertised_start_time['und']); $i++){
                    if($i > 0) $out .= " | "; // pipe between repeating dates
                    $out .= $event->field_advertised_start_time['und'][$i]['safe_value'];
                    if(isset($event->field_advertised_end_time['und'][$i])){
                        $out .= ' - ' . $event->field_advertised_end_time['und'][$i]['safe_value'];
                    }
                }
            }
            
            $out .= '</li>';
        
        }
        $out .= '</ul>';
    }
    
    // do the child venues
    if(isset($venue->children)){
        $out .= '<ul>';
        foreach($venue->children as $kid){
            render_venue($out, $kid);
        }
        $out .= '</ul>';
    }
    $out .= '</li>';
}




