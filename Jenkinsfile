pipeline
{
    agent { 
        docker { image "ubuntu:latest" } 
    }
    
    stages
    {
        stage("Building")
        { 
            steps
            {
                echo "hello from docker"

                git 'https://github.com/Bluwbee/connectedhomeip'

                dir("/build")
                {
                    sh 'cmake ..'
                    sh 'make' 
                }
            }
        }
    }
 } // pipeline