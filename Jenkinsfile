pipeline {
  agent {
    dockerfile {
      filename 'Dockerfile'
      dir 'ci'
      args '-v /jenkins_cache:/root'
    }
  }
  stages {
    stage('Build') {
      steps {
        sh 'ci/compile.sh'
      }
    }
  }
  post {
    success {
      archiveArtifacts artifacts: 'install/**/*.*', fingerprint: true
    }
  }
}

